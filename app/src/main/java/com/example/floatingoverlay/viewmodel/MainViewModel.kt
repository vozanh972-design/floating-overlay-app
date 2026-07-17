package com.example.floatingoverlay.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.example.floatingoverlay.model.ServiceState
import com.example.floatingoverlay.service.OverlayForegroundService
import com.example.floatingoverlay.util.PermissionUtils
import kotlinx.coroutines.launch

/**
 * ViewModel của MainActivity — theo đúng nguyên tắc MVVM:
 * Activity (View) chỉ hiển thị dữ liệu từ LiveData và forward sự kiện người dùng vào đây,
 * không tự chứa business logic kiểm tra quyền hay khởi động Service.
 */
class MainViewModel(application: Application) : AndroidViewModel(application) {

    private val _overlayGranted = MutableLiveData(false)
    val overlayGranted: LiveData<Boolean> get() = _overlayGranted

    private val _accessibilityGranted = MutableLiveData(false)
    val accessibilityGranted: LiveData<Boolean> get() = _accessibilityGranted

    private val _notificationGranted = MutableLiveData(false)
    val notificationGranted: LiveData<Boolean> get() = _notificationGranted

    private val _serviceState = MutableLiveData(ServiceState.STOPPED)
    val serviceState: LiveData<ServiceState> get() = _serviceState

    /** Gọi lại mỗi khi Activity onResume() để cập nhật trạng thái quyền mới nhất. */
    fun refreshPermissionState() {
        val context = getApplication<Application>().applicationContext
        _overlayGranted.value = PermissionUtils.hasOverlayPermission(context)
        _accessibilityGranted.value = PermissionUtils.isAccessibilityServiceEnabled(context)
        _notificationGranted.value = PermissionUtils.hasNotificationPermission(context)
    }

    fun toggleService() {
        val context = getApplication<Application>().applicationContext
        viewModelScope.launch {
            if (_serviceState.value == ServiceState.RUNNING) {
                OverlayForegroundService.requestStop(context)
                _serviceState.value = ServiceState.STOPPED
            } else {
                if (!PermissionUtils.allPermissionsGranted(context)) return@launch
                OverlayForegroundService.start(context)
                _serviceState.value = ServiceState.RUNNING
            }
        }
    }

    fun allPermissionsGranted(): Boolean =
        overlayGranted.value == true &&
            accessibilityGranted.value == true &&
            notificationGranted.value == true
}
