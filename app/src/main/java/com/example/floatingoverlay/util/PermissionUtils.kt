package com.example.floatingoverlay.util

import android.accessibilityservice.AccessibilityServiceInfo
import android.content.Context
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.view.accessibility.AccessibilityManager
import androidx.core.content.ContextCompat
import com.example.floatingoverlay.service.AppAccessibilityService

/**
 * Tập hợp hàm kiểm tra quyền — tách riêng khỏi Activity/ViewModel để dễ test
 * và tái sử dụng ở nhiều nơi (Service cũng cần kiểm tra Overlay trước khi vẽ view).
 */
object PermissionUtils {

    /** Kiểm tra quyền vẽ overlay (SYSTEM_ALERT_WINDOW) - API đặc thù Android 6.0+. */
    fun hasOverlayPermission(context: Context): Boolean {
        return Settings.canDrawOverlays(context)
    }

    /** Mở màn hình cài đặt hệ thống để người dùng cấp quyền Overlay. */
    fun overlayPermissionIntent(context: Context) =
        android.content.Intent(
            Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
            Uri.parse("package:${context.packageName}")
        )

    /**
     * Kiểm tra Accessibility Service của app đã được BẬT trong
     * Settings > Accessibility hay chưa. Không có API "request" trực tiếp —
     * phải điều hướng người dùng tới màn hình Cài đặt hệ thống.
     */
    fun isAccessibilityServiceEnabled(context: Context): Boolean {
        val am = context.getSystemService(Context.ACCESSIBILITY_SERVICE) as? AccessibilityManager
            ?: return false
        val enabledServices = am.getEnabledAccessibilityServiceList(
            AccessibilityServiceInfo.FEEDBACK_ALL_MASK
        )
        return enabledServices.any { info ->
            info.resolveInfo.serviceInfo.packageName == context.packageName &&
                info.resolveInfo.serviceInfo.name == AppAccessibilityService::class.java.name
        }
    }

    /** Mở màn hình cài đặt Accessibility. */
    fun accessibilitySettingsIntent() =
        android.content.Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)

    /**
     * Từ Android 13 (API 33) trở lên, POST_NOTIFICATIONS là runtime permission bắt buộc
     * để Foreground Service hiển thị thông báo. Trước đó luôn coi như đã cấp.
     */
    fun hasNotificationPermission(context: Context): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ContextCompat.checkSelfPermission(
                context,
                android.Manifest.permission.POST_NOTIFICATIONS
            ) == PackageManager.PERMISSION_GRANTED
        } else {
            true
        }
    }

    /** Tất cả quyền cần thiết đã sẵn sàng để bật service. */
    fun allPermissionsGranted(context: Context): Boolean {
        return hasOverlayPermission(context) &&
            isAccessibilityServiceEnabled(context) &&
            hasNotificationPermission(context)
    }
}
