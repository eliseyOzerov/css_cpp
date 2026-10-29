// swift-tools-version:5.9
// css_cpp for Swift and any SwiftPM host. The C++ is used through C++ interop
// (`.interoperabilityMode(.Cxx)` in the Swift target that imports it).
import PackageDescription

let package = Package(
  name: "css_cpp",
  // The oldest systems it is built and tested for, as math_cpp's.
  platforms: [.macOS("13.3"), .iOS("16.3")],
  products: [.library(name: "CssCpp", targets: ["CssCpp"])],
  targets: [.target(name: "CssCpp", path: "Css", publicHeadersPath: ".")],
  cxxLanguageStandard: .cxx20
)
