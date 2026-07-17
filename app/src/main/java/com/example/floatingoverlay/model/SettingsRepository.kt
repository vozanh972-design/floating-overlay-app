package com.example.floatingoverlay.model

import android.content.Context
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

// Extension property tạo 1 instance DataStore duy nhất cho toàn app
private val Context.dataStore by preferencesDataStore(name = "overlay_settings")

/**
 * Repository (tầng Model trong MVVM) — nguồn dữ liệu duy nhất (Single Source of Truth)
 * cho cấu hình overlay. ViewModel chỉ giao tiếp qua đây, không đụng trực tiếp DataStore.
 */
class SettingsRepository(private val context: Context) {

    private object Keys {
        val OPACITY = intPreferencesKey("opacity_percent")
        val BUTTON_SIZE = intPreferencesKey("button_size_dp")
        val VIBRATION = booleanPreferencesKey("vibration_enabled")
        val AUTO_START = booleanPreferencesKey("auto_start_enabled")
    }

    /** Flow phát ra OverlaySettings mỗi khi có thay đổi — ViewModel sẽ collect cái này. */
    val settingsFlow: Flow<OverlaySettings> = context.dataStore.data.map { prefs ->
        OverlaySettings(
            opacityPercent = prefs[Keys.OPACITY] ?: 100,
            buttonSizeDp = prefs[Keys.BUTTON_SIZE] ?: 56,
            vibrationEnabled = prefs[Keys.VIBRATION] ?: true,
            autoStartEnabled = prefs[Keys.AUTO_START] ?: false
        )
    }

    suspend fun updateOpacity(value: Int) {
        context.dataStore.edit { it[Keys.OPACITY] = value }
    }

    suspend fun updateButtonSize(value: Int) {
        context.dataStore.edit { it[Keys.BUTTON_SIZE] = value }
    }

    suspend fun updateVibration(enabled: Boolean) {
        context.dataStore.edit { it[Keys.VIBRATION] = enabled }
    }

    suspend fun updateAutoStart(enabled: Boolean) {
        context.dataStore.edit { it[Keys.AUTO_START] = enabled }
    }

    companion object {
        @Volatile private var INSTANCE: SettingsRepository? = null

        fun getInstance(context: Context): SettingsRepository =
            INSTANCE ?: synchronized(this) {
                INSTANCE ?: SettingsRepository(context.applicationContext).also { INSTANCE = it }
            }
    }
}
