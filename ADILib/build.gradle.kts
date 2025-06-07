plugins {
    alias(libs.plugins.android.library)
}

val defaultCFlags = arrayOf(
        "-Wall", "-Wextra",
        "-fno-rtti", "-fno-exceptions",
        "-fno-stack-protector", "-fomit-frame-pointer",
        "-Wno-builtin-macro-redefined", "-D__FILE__=__FILE_NAME__"
)


android {

    defaultConfig {

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                abiFilters.add("arm64-v8a")
//                abiFilters.add("armeabi-v7a")
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
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        prefab = true
    }
}