package com.example.floatingoverlay.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.asLiveData
import androidx.lifecycle.viewModelScope
import com.example.floatingoverlay.model.OverlaySettings
import com.example.floatingoverlay.model.SettingsRepository
import kotlinx.coroutines.launch

/**
 * ViewModel cho SettingsActivity. Toàn bộ đọc/ghi đi qua [SettingsRepository]
 * (tầng Model), ViewModel chỉ expose LiveData + hàm cập nhật cho View gọi.
 */
class SettingsViewModel(private val repository: SettingsRepository) : ViewModel() {

    val settings: LiveData<OverlaySettings> = repository.settingsFlow.asLiveData()

    fun setOpacity(value: Int) = viewModelScope.launch {
        repository.updateOpacity(value)
    }

    fun setButtonSize(value: Int) = viewModelScope.launch {
        repository.updateButtonSize(value)
    }

    fun setVibration(enabled: Boolean) = viewModelScope.launch {
        repository.updateVibration(enabled)
    }

    fun setAutoStart(enabled: Boolean) = viewModelScope.launch {
        repository.updateAutoStart(enabled)
    }

    /** Factory thủ công vì SettingsViewModel cần Repository trong constructor (không có Hilt). */
    class Factory(private val repository: SettingsRepository) : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T {
            if (modelClass.isAssignableFrom(SettingsViewModel::class.java)) {
                return SettingsViewModel(repository) as T
            }
            throw IllegalArgumentException("Unknown ViewModel class: $modelClass")
        }
    }
}
