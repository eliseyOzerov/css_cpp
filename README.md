# css_cpp

CSS colours (hex, rgb(), hsl(), transparent and the 147 named colours), declaration blocks such as an inline `style`, and style sheets of simple selectors applied by specificity. Written from CSS Color Module Level 3 and CSS Selectors Level 3.

C++20 with only the standard library. One of a family of small libraries built and used the same
way: [math_cpp](https://github.com/eliseyOzerov/math_cpp), xml_cpp, deflate_cpp, css_cpp and
opentype_cpp.

## Build and test

```bash
make test
```

builds `build/lib/libcss_cpp.a` and runs the unit tests.

## Using it

**make or any build system:** put the folder that holds `css_cpp` on the include path, include
`"css_cpp/Css/<Header>.h"`, and link `libcss_cpp.a`.

**SwiftPM:** the product is `CssCpp`.

```swift
.package(path: "../css_cpp"),
// ...
.target(name: "App", dependencies: [.product(name: "CssCpp", package: "css_cpp")],
        swiftSettings: [.interoperabilityMode(.Cxx)])
```

Everything is in the C++ namespace `editor`, as in math_cpp. Values come back in the format's own
terms; turning them into a host's types is the host's job.

## License

MIT, see [LICENSE](LICENSE).
