import com.android.build.gradle.LibraryExtension
import java.io.ByteArrayOutputStream

// Top-level build file where you can add configuration options common to all sub-projects/modules.
plugins {
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.android.application) apply false
}


//val gitCommitCount = "git rev-list HEAD --count".execute().toInt()
val moduleId by extra("syzuel")
val moduleName by extra("Zygisk syzuel")
val verName by extra("v4-0.9.1.1")
