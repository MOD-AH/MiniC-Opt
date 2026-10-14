// Generates Cppify's brand assets from the two source images in source/.
//
//   swiftc -O make-assets.swift -o /tmp/make-assets && /tmp/make-assets <cppify dir> && ./package-assets.sh
//
// Every output size is resampled straight from the full-resolution source
// with an area-average (box) filter in premultiplied alpha. Area averaging is
// what keeps the fine stripe pattern from turning into moire or jagged
// edges when it is shrunk 20-80x; chaining halvings would blur each step.
import Foundation
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

struct RGBA { var w: Int, h: Int, px: [Float] }   // premultiplied, 0...1, row 0 = top

func load(_ path: String) -> RGBA {
    let src = CGImageSourceCreateWithURL(URL(fileURLWithPath: path) as CFURL, nil)!
    let img = CGImageSourceCreateImageAtIndex(src, 0, nil)!
    let w = img.width, h = img.height
    var buf = [UInt8](repeating: 0, count: w * h * 4)
    let ctx = CGContext(data: &buf, width: w, height: h, bitsPerComponent: 8, bytesPerRow: w * 4,
                        space: CGColorSpace(name: CGColorSpace.sRGB)!,
                        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
    ctx.draw(img, in: CGRect(x: 0, y: 0, width: w, height: h))
    return RGBA(w: w, h: h, px: buf.map { Float($0) / 255 })
}

func save(_ im: RGBA, _ path: String) {
    var buf = im.px.map { UInt8(max(0, min(255, ($0 * 255).rounded()))) }
    let ctx = CGContext(data: &buf, width: im.w, height: im.h, bitsPerComponent: 8, bytesPerRow: im.w * 4,
                        space: CGColorSpace(name: CGColorSpace.sRGB)!,
                        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
    let dest = CGImageDestinationCreateWithURL(URL(fileURLWithPath: path) as CFURL,
                                               UTType.png.identifier as CFString, 1, nil)!
    CGImageDestinationAddImage(dest, ctx.makeImage()!, nil)
    precondition(CGImageDestinationFinalize(dest), "could not write \(path)")
}

/// Exact area-average resample of the source rectangle (sx, sy, sw, sh) into
/// a dw x dh image: every destination pixel is the coverage-weighted mean of
/// the source pixels under it, including fractional edge pixels.
func resample(_ s: RGBA, sx: Double, sy: Double, sw: Double, sh: Double, dw: Int, dh: Int) -> RGBA {
    var out = [Float](repeating: 0, count: dw * dh * 4)
    let fx = sw / Double(dw), fy = sh / Double(dh)
    for oy in 0..<dh {
        let y0 = sy + Double(oy) * fy, y1 = y0 + fy
        for ox in 0..<dw {
            let x0 = sx + Double(ox) * fx, x1 = x0 + fx
            var acc: (Double, Double, Double, Double) = (0, 0, 0, 0)
            var area = 0.0
            var yy = Int(floor(y0))
            while Double(yy) < y1 {
                let wy = min(y1, Double(yy + 1)) - max(y0, Double(yy))
                if yy >= 0 && yy < s.h && wy > 0 {
                    var xx = Int(floor(x0))
                    while Double(xx) < x1 {
                        let wx = min(x1, Double(xx + 1)) - max(x0, Double(xx))
                        if xx >= 0 && xx < s.w && wx > 0 {
                            let i = (yy * s.w + xx) * 4, k = wx * wy
                            acc.0 += Double(s.px[i]) * k; acc.1 += Double(s.px[i+1]) * k
                            acc.2 += Double(s.px[i+2]) * k; acc.3 += Double(s.px[i+3]) * k
                        }
                        area += max(0, wx) * wy
                        xx += 1
                    }
                }
                yy += 1
            }
            let o = (oy * dw + ox) * 4
            if area > 0 {
                out[o] = Float(acc.0 / area); out[o+1] = Float(acc.1 / area)
                out[o+2] = Float(acc.2 / area); out[o+3] = Float(acc.3 / area)
            }
        }
    }
    return RGBA(w: dw, h: dh, px: out)
}

func smoothstep(_ e0: Float, _ e1: Float, _ x: Float) -> Float {
    let t = max(0, min(1, (x - e0) / (e1 - e0))); return t * t * (3 - 2 * t)
}

// ---------------------------------------------------------------------------
// App icon
// ---------------------------------------------------------------------------
// The art is a glowing rounded square on black. Inside the square stays
// opaque; outside it, black becomes transparent while the orange glow stays,
// because on a black background a pixel's colour already IS its
// premultiplied colour with alpha = its brightest channel.
func makeIcon(_ src: RGBA) -> (RGBA, (Double, Double, Double)) {
    // Rim measured on the 1254 px source (left 161, right 1090, top 148,
    // bottom 1037), corner radius ~230. Inset slightly so the bright rim
    // itself is carried by the glow rule and keeps its soft outer edge.
    let rx0: Float = 167, rx1: Float = 1084, ry0: Float = 154, ry1: Float = 1031, radius: Float = 224
    let cx = (rx0 + rx1) / 2, cy = (ry0 + ry1) / 2, hx = (rx1 - rx0) / 2, hy = (ry1 - ry0) / 2
    var p = src.px
    for y in 0..<src.h {
        for x in 0..<src.w {
            // Signed distance to the rounded rectangle (negative inside).
            let qx = abs(Float(x) + 0.5 - cx) - (hx - radius), qy = abs(Float(y) + 0.5 - cy) - (hy - radius)
            let outside = (max(qx, 0) * max(qx, 0) + max(qy, 0) * max(qy, 0)).squareRoot()
            let sd = outside + min(max(qx, qy), 0) - radius
            let inside = 1 - smoothstep(-1.5, 1.5, sd)           // 1.5 px feathered edge, no stair-steps
            let i = (y * src.w + x) * 4
            let glow = max(p[i], p[i+1], p[i+2])
            p[i+3] = inside + (1 - inside) * glow
        }
    }
    let shaped = RGBA(w: src.w, h: src.h, px: p)
    // Apple's macOS grid: the shape occupies 824 of 1024. Crop a square
    // around the shape's centre sized so its width lands exactly on that.
    let side = Double(rx1 - rx0 + 12) / 824.0 * 1024.0
    return (shaped, (Double(cx) - side / 2, Double(cy) - side / 2, side))
}

// ---------------------------------------------------------------------------
// macOS 26 icon layer
// ---------------------------------------------------------------------------
// macOS 26 draws an app icon itself: its own rounded-square mask, glass rim
// and the Default / Dark / Clear / Tinted styles. A finished flat picture
// (the .icns above) gets shrunk into a grey "legacy" square instead. So the
// real icon is an Icon Composer document: a dark fill plus ONE foreground
// layer -- the "CP" with its glow -- lifted off the art's black interior.
func makeIconLayer(_ src: RGBA, mono: Bool = false) -> RGBA {
    // Same rim as makeIcon. Inset well inside it so the art's own glossy rim
    // and reflections stay out: the system draws its own.
    let rx0: Float = 161, rx1: Float = 1090, ry0: Float = 148, ry1: Float = 1037
    let inset: Float = 48, radius: Float = 230 - 48
    let cx = (rx0 + rx1) / 2, cy = (ry0 + ry1) / 2
    let hx = (rx1 - rx0) / 2 - inset, hy = (ry1 - ry0) / 2 - inset
    var p = src.px
    for y in 0..<src.h {
        for x in 0..<src.w {
            let qx = abs(Float(x) + 0.5 - cx) - (hx - radius), qy = abs(Float(y) + 0.5 - cy) - (hy - radius)
            let outside = (max(qx, 0) * max(qx, 0) + max(qy, 0) * max(qy, 0)).squareRoot()
            let sd = outside + min(max(qx, qy), 0) - radius
            let keep = 1 - smoothstep(-12, 0, sd)              // fade out before the inset edge
            let i = (y * src.w + x) * 4
            // On black, colour is already premultiplied and alpha is the
            // brightest channel; a small floor drops the glass's dark haze.
            let m = max(p[i], p[i+1], p[i+2])
            if mono {
                // Clear / Tinted styles build glass from this layer's shape.
                // Faint glow there turns into speckle, so keep only the
                // letters' strokes -- any colour -- as clean white.
                let a = smoothstep(0.30, 0.65, m) * keep
                p[i] = a; p[i+1] = a; p[i+2] = a; p[i+3] = a
            } else {
                let a = max(0, (m - 0.06) / 0.94) * keep
                let k = m > 0 ? a / m : 0
                p[i] *= k; p[i+1] *= k; p[i+2] *= k; p[i+3] = a
            }
        }
    }
    // The shape's width maps to the full 1024 canvas: Icon Composer layers
    // are full-bleed and the system applies the mask.
    let side = Double(rx1 - rx0)
    return resample(RGBA(w: src.w, h: src.h, px: p),
                    sx: Double(cx) - side / 2, sy: Double(cy) - side / 2, sw: side, sh: side, dw: 1024, dh: 1024)
}

// ---------------------------------------------------------------------------
// Title-bar wordmark
// ---------------------------------------------------------------------------
// The art is black stripes with orange highlights on a transparent
// background. On the dark title bar black would vanish, so the neutral
// stripes are turned white while the orange stays as it is -- the same look
// as the icon.
func makeWordmark(_ src: RGBA) -> (white: RGBA, orange: RGBA, box: (Double, Double, Double, Double)) {
    var p = src.px
    var q = [Float](repeating: 0, count: src.px.count)   // the orange highlights, kept apart
    var colStrong = [Int](repeating: 0, count: src.w), rowStrong = [Int](repeating: 0, count: src.h)
    for y in 0..<src.h {
        for x in 0..<src.w {
            let i = (y * src.w + x) * 4
            // The source carries real transparency: colour is premultiplied
            // by its own alpha, so un-premultiply rather than un-mix from white.
            let a = p[i+3]
            guard a > 0.004 else { p[i] = 0; p[i+1] = 0; p[i+2] = 0; p[i+3] = 0; continue }
            let u = [p[i] / a, p[i+1] / a, p[i+2] / a].map { max(0, min(1, $0)) }
            let sat = (u.max()! - u.min()!)
            let neutral = 1 - smoothstep(0.12, 0.45, sat)       // grey/black -> white
            // Split each pixel: the whole of it becomes white letter; its
            // coloured share also goes to the highlight layer with its colour.
            // The letter SHAPE uses all the ink (white or orange); only the
            // glow layer is restricted to the orange share. Otherwise orange
            // parts of a stroke leave holes in the white letter.
            let oa = a * (1 - neutral)
            p[i] = a; p[i+1] = a; p[i+2] = a; p[i+3] = a
            q[i] = u[0] * oa; q[i+1] = u[1] * oa; q[i+2] = u[2] * oa; q[i+3] = oa
            if a > 0.5 { colStrong[x] += 1; rowStrong[y] += 1 }
        }
    }
    // The letters are where many strong pixels line up; the source also has
    // faint smudges near its edges that must not stretch the box.
    let minX = colStrong.firstIndex { $0 > 8 }!, maxX = colStrong.lastIndex { $0 > 8 }!
    let minY = rowStrong.firstIndex { $0 > 8 }!, maxY = rowStrong.lastIndex { $0 > 8 }!
    let pad = Double(maxY - minY) * 0.04
    let box = (Double(minX) - pad, Double(minY) - pad, Double(maxX - minX) + 2 * pad, Double(maxY - minY) + 2 * pad)
    return (RGBA(w: src.w, h: src.h, px: p), RGBA(w: src.w, h: src.h, px: q), box)
}

/// After shrinking, a letter is ~50% covered by stripes, which reads as a
/// washed-out grey. Lift coverage so letter bodies come out solid while the
/// anti-aliased edges keep their smooth ramp.
func solidify(_ im: RGBA, gain: Float) -> RGBA {
    var p = im.px
    for i in stride(from: 0, to: p.count, by: 4) {
        let a = p[i+3]; guard a > 0 else { continue }
        let na = min(1, a * gain), k = na / a
        p[i] *= k; p[i+1] *= k; p[i+2] *= k; p[i+3] = na
    }
    return RGBA(w: im.w, h: im.h, px: p)
}

/// 3x3 binomial blur (1-2-1). At title-bar size a one-pixel orange streak
/// reads as noise; softening it by half a pixel turns it back into a glow.
func soften(_ im: RGBA) -> RGBA {
    var out = im.px
    let k: [Float] = [1, 2, 1]
    for y in 0..<im.h {
        for x in 0..<im.w {
            var acc: [Float] = [0, 0, 0, 0], wsum: Float = 0
            for dy in -1...1 { for dx in -1...1 {
                let yy = y + dy, xx = x + dx
                guard yy >= 0, yy < im.h, xx >= 0, xx < im.w else { continue }
                let wgt = k[dy + 1] * k[dx + 1], i = (yy * im.w + xx) * 4
                for ch in 0..<4 { acc[ch] += im.px[i + ch] * wgt }
                wsum += wgt
            } }
            let o = (y * im.w + x) * 4
            for ch in 0..<4 { out[o + ch] = acc[ch] / wsum }
        }
    }
    return RGBA(w: im.w, h: im.h, px: out)
}

/// Porter-Duff "over" in premultiplied alpha, with the top layer's strength scaled.
func over(_ top: RGBA, _ bottom: RGBA, strength: Float) -> RGBA {
    var out = bottom.px
    for i in stride(from: 0, to: out.count, by: 4) {
        let ta = top.px[i+3] * strength
        for ch in 0..<3 { out[i+ch] = top.px[i+ch] * strength + bottom.px[i+ch] * (1 - ta) }
        out[i+3] = ta + bottom.px[i+3] * (1 - ta)
    }
    return RGBA(w: bottom.w, h: bottom.h, px: out)
}

// ---------------------------------------------------------------------------
let appy = CommandLine.arguments[1]          // the cppify/ directory
let fm = FileManager.default

// Icon
let (shaped, crop) = makeIcon(load("\(appy)/branding/source/icon.webp"))
let iconset = "\(appy)/branding/out/AppIcon.iconset"
try? fm.removeItem(atPath: iconset)
try! fm.createDirectory(atPath: iconset, withIntermediateDirectories: true)
for base in [16, 32, 128, 256, 512] {
    for scale in [1, 2] {
        let px = base * scale
        let im = resample(shaped, sx: crop.0, sy: crop.1, sw: crop.2, sh: crop.2, dw: px, dh: px)
        save(im, "\(iconset)/icon_\(base)x\(base)\(scale == 2 ? "@2x" : "").png")
    }
}
print("icon: wrote \(iconset)")

// macOS 26 icon document
let iconDoc = "\(appy)/branding/AppIcon.icon"
try? fm.removeItem(atPath: iconDoc)
try! fm.createDirectory(atPath: "\(iconDoc)/Assets", withIntermediateDirectories: true)
let iconSrc = load("\(appy)/branding/source/icon.webp")
save(makeIconLayer(iconSrc), "\(iconDoc)/Assets/cp.png")
save(makeIconLayer(iconSrc, mono: true), "\(iconDoc)/Assets/cp-mono.png")
let iconJSON = """
{
  "fill" : {
    "solid" : "extended-srgb:0.03500,0.03500,0.04000,1.00000"
  },
  "groups" : [
    {
      "layers" : [
        {
          "image-name-specializations" : [
            { "value" : "cp.png" },
            { "appearance" : "tinted", "value" : "cp-mono.png" }
          ],
          "name" : "cp"
        }
      ]
    }
  ],
  "supported-platforms" : {
    "squares" : "shared"
  }
}
"""
try! iconJSON.write(toFile: "\(iconDoc)/icon.json", atomically: true, encoding: .utf8)
print("icon document: wrote \(iconDoc)")

// Wordmark: 15 pt tall in the title bar (just above the 12-13 pt traffic lights).
let (whiteLayer, orangeLayer, box) = makeWordmark(load("\(appy)/branding/source/wordmark.webp"))
let heightPt = 15.0
let widthPt = (box.2 / box.3 * heightPt).rounded()
for scale in [1, 2] {   // Macs are 1x or 2x; nothing ships a 3x display
    let dw = Int(widthPt) * scale, dh = Int(heightPt) * scale
    let letters = solidify(resample(whiteLayer, sx: box.0, sy: box.1, sw: box.2, sh: box.3, dw: dw, dh: dh), gain: 2.2)
    // The orange goes UNDER the letters and is spread a little, so it reads
    // as an edge glow around crisp white letters -- the icon's look --
    // rather than as stains on top of them.
    var glow = solidify(resample(orangeLayer, sx: box.0, sy: box.1, sw: box.2, sh: box.3, dw: dw, dh: dh), gain: 3.0)
    // Repeated 1-2-1 passes approximate a Gaussian; the radius scales with
    // the output scale so @1x, @2x and @3x have the same glow in points.
    for _ in 0..<(3 * scale) { glow = soften(glow) }
    glow = solidify(glow, gain: 2.2)
    let im = over(letters, glow, strength: 1)
    save(im, "\(appy)/branding/out/wordmark\(scale == 1 ? "" : "@\(scale)x").png")
}
print("wordmark: \(Int(widthPt))x\(Int(heightPt)) pt, letters box \(box)")
