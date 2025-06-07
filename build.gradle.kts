import com.android.build.gradle.LibraryExtension
import java.io.ByteArrayOutputStream

// Top-level build file where you can add configuration options common to all sub-projects/modules.
plugins {
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.android.application) apply false

}

val androidCompileSdkVersion by extra(34)
val androidBuildToolsVersion by extra("35.0.0")
val androidCompileNdkVersion by extra( "27.0.12077973")
val androidMinSdkVersion by extra(27)
val androidTargetSdkVersion by extra(35)

fun Project.configureBaseExtension() {
    extensions.findByType(LibraryExtension::class)?.run {
        namespace = "com.hepta.zygiskADI"
        compileSdk = 34
        ndkVersion = androidCompileNdkVersion
        buildToolsVersion = androidBuildToolsVersion

        defaultConfig {
            minSdk = androidMinSdkVersion
        }

        lint {
            abortOnError = true
        }
    }
}

subprojects {
    plugins.withId("com.android.library") {
        configureBaseExtension()
    }
}
