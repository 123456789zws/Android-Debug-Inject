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
    namespace = "com.hepta.zygisk"
    compileSdk = 34

    defaultConfig {
        minSdk = 27

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cFlags("-std=c18", *defaultCFlags)
                cppFlags("-std=c++20", *defaultCFlags)
                abiFilters.add("arm64-v8a")
                abiFilters.add("armeabi-v7a")

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
    ndkVersion = "27.0.12077973"

}

dependencies {

//    implementation(libs.appcompat)
//    implementation(libs.material)
//    testImplementation(libs.junit)
//    androidTestImplementation(libs.ext.junit)
//    androidTestImplementation(libs.espresso.core)
}