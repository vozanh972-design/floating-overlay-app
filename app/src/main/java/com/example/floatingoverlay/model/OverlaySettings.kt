package com.example.floatingoverlay.model

/**
 * Data class chứa toàn bộ cấu hình có thể tuỳ chỉnh của nút nổi.
 * Được lưu trữ bền vững qua [com.example.floatingoverlay.model.SettingsRepository].
 */
data class OverlaySettings(
    val opacityPercent: Int = 100,   // 30..100 (%)
    val buttonSizeDp: Int = 56,      // 40..80 dp
    val vibrationEnabled: Boolean = true,
    val autoStartEnabled: Boolean = false
) {
    /** Alpha dùng trực tiếp cho View.alpha (0f..1f) */
    fun alphaFloat(): Float = (opacityPercent.coerceIn(30, 100)) / 100f
}

/** Trạng thái vòng đời của dịch vụ nút nổi, dùng chung giữa Service và ViewModel. */
enum class ServiceState {
    STOPPED,
    RUNNING
}
