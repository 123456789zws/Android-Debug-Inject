plugins {
    alias(libs.plugins.android.library)
}


val defaultCFlags = arrayOf(
    "-Wall", "-Wextra",
    "-fno-rtti", "-fno-exceptions",
    "-fno-stack-protector", "-fomit-frame-pointer",
    "-Wno-builtin-macro-redefined", "-D__FILE__=__FILE_NAME__"
)

val releaseFlags = arrayOf(
    "-Oz", "-flto",
    "-Wno-unused", "-Wno-unused-parameter",
    "-fvisibility=hidden", "-fvisibility-inlines-hidden",
    "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
    "-Wl,--exclude-libs,ALL", "-Wl,--gc-sections", "-Wl,--strip-all"
)

android {

    defaultConfig {

        externalNativeBuild {
            cmake {
//                arguments += "-DANDROID_STL=none" //关闭原生stl  如何打卡将关闭ndk自带的stl,需要在导入stl库
                cFlags("-std=c18", *defaultCFlags)
                cppFlags("-std=c++20", *defaultCFlags)
//                abiFilters.add("armeabi-v7a")
                abiFilters.add("arm64-v8a")

            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild.cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
    }

    buildFeatures {
        prefab = true
    }
}
