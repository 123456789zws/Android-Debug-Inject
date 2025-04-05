import android.databinding.tool.ext.capitalizeUS
import org.ajoberstar.grgit.Grgit

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.grgit)
}
android{

    namespace = "com.singularity.zygisk"
    compileSdk = 34
    buildFeatures {
        resValues = false
        buildConfig = false

    }
}
var git = Grgit.open{  dir = rootProject.rootDir  } // 明确指定目录


val moduleId by extra("dZygisk")
val moduleName by extra("Android Debug Inject Zygisk")
val verName by extra("v0-0.1")
val verCode by extra(git.log().size)
val commitHash by extra(git.head().abbreviatedId)


androidComponents.onVariants { variant ->
    val variantLowered = variant.name.lowercase()
    val variantCapped = variant.name.capitalizeUS()
    val buildTypeLowered = variant.buildType?.lowercase()
    val moduleDir = layout.buildDirectory.dir("outputs/module/$variantLowered")
    val zipFileName = "$moduleName-$verName-$verCode-$commitHash-$buildTypeLowered.zip".replace(' ', '-')
    val prepareModuleFilesTask = task<Sync>("prepareModuleFiles$variantCapped") {

        dependsOn(
            ":DebugInject:externalNativeBuild$variantCapped",
            ":Zygisk:externalNativeBuild$variantCapped",
            ":ADILib:externalNativeBuild$variantCapped",
        )
        into(moduleDir)
        from("$projectDir/src") {
            exclude("module.prop")
//            filter<FixCrLfFilter>("eol" to FixCrLfFilter.CrLf.newInstance("lf"))
        }
        from("$projectDir/src") {
            include("module.prop")
            expand(
                "moduleId" to moduleId,
                "moduleName" to moduleName,
                "versionName" to "$verName($variantLowered)",
                "versionCode" to verCode
            )
        }

        into("lib/arm64-v8a"){
            from(project(":Zygisk").layout.buildDirectory.file("intermediates/cmake/$variantLowered/obj/arm64-v8a/libzygisk.so"))
            from(project(":ADILib").layout.buildDirectory.file("intermediates/cmake/$variantLowered/obj/arm64-v8a/libDrmHook.so"))
        }
        into("bin"){
            from(project(":Zygisk").layout.buildDirectory.file("intermediates/cmake/$variantLowered/obj/arm64-v8a/zygiskd"))
            from(project(":DebugInject").layout.buildDirectory.file("intermediates/cmake/$variantLowered/obj/arm64-v8a/ptraceInit"))

        }


    }

    val zipTask = task<Zip>("zip$variantCapped") {
        group = "module"
        dependsOn(prepareModuleFilesTask)
        archiveFileName.set(zipFileName)
        destinationDirectory.set(layout.buildDirectory.file("outputs/release").get().asFile)
        from(moduleDir)
    }


}