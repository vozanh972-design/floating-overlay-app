package com.example.floatingoverlay

import android.app.Application
import com.example.floatingoverlay.model.SettingsRepository

/**
 * Application class — nơi khởi tạo các dependency singleton dùng chung toàn app
 * (thay thế cho DI framework như Hilt trong project quy mô nhỏ này).
 */
class OverlayApplication : Application() {

    /** Repository dùng chung giữa MainActivity, SettingsActivity và OverlayForegroundService. */
    val settingsRepository: SettingsRepository by lazy {
        SettingsRepository.getInstance(this)
    }

    override fun onCreate() {
        super.onCreate()
        // Điểm mở rộng: khởi tạo logging, crash reporting... tại đây nếu cần.
    }
}
