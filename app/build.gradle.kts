plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.example.floatingoverlay"
    // compileSdk 35 = Android 15
    compileSdk = 35

    defaultConfig {
        applicationId = "com.example.floatingoverlay"
        // minSdk 29 = Android 10 (yêu cầu tối thiểu của đề bài)
        minSdk = 29
        // targetSdk 35 = Android 15 (mới nhất)
        targetSdk = 35
        versionCode = 1
        versionName = "1.0.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        debug {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        viewBinding = true
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }
}

dependencies {
    // Core + Kotlin
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")

    // Material Design 3
    implementation("com.google.android.material:material:1.12.0")

    // Lifecycle / ViewModel / LiveData (MVVM)
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.8.4")
    implementation("androidx.lifecycle:lifecycle-livedata-ktx:2.8.4")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.4")
    implementation("androidx.lifecycle:lifecycle-service:2.8.4")

    // Coroutines
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")

    // DataStore (thay SharedPreferences để lưu Settings - khuyến nghị hiện đại)
    implementation("androidx.datastore:datastore-preferences:1.1.1")

    // ConstraintLayout cho layout overlay
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")

    // CardView cho panel điều khiển nổi
    implementation("androidx.cardview:cardview:1.0.0")

    // Testing
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
}
