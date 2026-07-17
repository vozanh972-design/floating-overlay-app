package com.example.floatingoverlay.service

import android.accessibilityservice.AccessibilityService
import android.content.Intent
import android.util.Log
import android.view.accessibility.AccessibilityEvent
import com.example.floatingoverlay.model.ServiceState

/**
 * Accessibility Service của ứng dụng.
 *
 * QUAN TRỌNG: Đây chỉ là lớp "cổng vào" hệ thống Accessibility — nơi Android
 * cấp cho app các quyền đặc biệt (canPerformGestures, canRetrieveWindowContent...).
 * Toàn bộ logic hiển thị UI overlay được tách sang [OverlayForegroundService] +
 * [FloatingBubbleController] để:
 *   - Dễ kiểm thử độc lập (không phụ thuộc AccessibilityService lifecycle).
 *   - Cho phép Start/Stop "chức năng nổi" mà không cần bật/tắt cả Accessibility Service
 *     (người dùng chỉ cần cấp quyền Accessibility 1 lần duy nhất).
 *
 * Nếu ứng dụng của bạn cần lắng nghe sự kiện hệ thống (đổi cửa sổ, nội dung màn hình...),
 * hãy hiện thực onAccessibilityEvent() bên dưới.
 */
class AppAccessibilityService : AccessibilityService() {

    companion object {
        private const val TAG = "AppAccessibilityService"

        // Trạng thái tĩnh để các thành phần khác (ViewModel, Activity) biết
        // service đã được hệ thống bind & kích hoạt hay chưa.
        @Volatile
        var isServiceConnected: Boolean = false
            private set

        @Volatile
        var instance: AppAccessibilityService? = null
            private set
    }

    override fun onServiceConnected() {
        super.onServiceConnected()
        isServiceConnected = true
        instance = this
        Log.d(TAG, "Accessibility Service connected")
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {
        // TODO: Xử lý sự kiện trợ năng tại đây nếu cần (vd: theo dõi window state đổi).
        // Để trống theo mặc định — mọi automation cụ thể do bạn tự bổ sung
        // tuỳ mục đích sử dụng hợp lệ của ứng dụng.
    }

    override fun onInterrupt() {
        Log.w(TAG, "Accessibility Service interrupted")
    }

    override fun onUnbind(intent: Intent?): Boolean {
        isServiceConnected = false
        instance = null
        // Đảm bảo overlay cũng được dọn dẹp khi Accessibility bị tắt từ Settings
        OverlayForegroundService.requestStop(applicationContext)
        return super.onUnbind(intent)
    }

    /** Helper để ViewModel/Activity truy vấn nhanh trạng thái hiện tại. */
    fun currentState(): ServiceState =
        if (isServiceConnected) ServiceState.RUNNING else ServiceState.STOPPED
}
