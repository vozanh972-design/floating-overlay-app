package com.example.floatingoverlay.util

import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import kotlin.math.abs

/**
 * OnTouchListener tái sử dụng cho mọi view được add vào WindowManager (overlay).
 * Nhiệm vụ:
 *  1. Cho phép kéo thả view tự do trên màn hình bằng cách cập nhật LayoutParams.x/y.
 *  2. Phân biệt được "tap" (nhấn ngắn, không di chuyển) và "drag" (kéo),
 *     vì WindowManager không có click listener built-in như View thường.
 *
 * @param windowManager WindowManager hệ thống dùng để updateViewLayout khi kéo.
 * @param layoutParams LayoutParams hiện tại của view (loại WindowManager.LayoutParams).
 * @param onClick callback khi phát hiện là tap (không phải kéo).
 * @param onDragEnd callback khi kết thúc một cú kéo (dùng để snap mép màn hình nếu cần).
 */
class DraggableTouchListener(
    private val windowManager: WindowManager,
    private val layoutParams: WindowManager.LayoutParams,
    private val onClick: () -> Unit,
    private val onDragEnd: ((x: Int, y: Int) -> Unit)? = null
) : View.OnTouchListener {

    private var initialX = 0
    private var initialY = 0
    private var initialTouchX = 0f
    private var initialTouchY = 0f
    private var downTimeMs = 0L

    override fun onTouch(view: View, event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                initialX = layoutParams.x
                initialY = layoutParams.y
                initialTouchX = event.rawX
                initialTouchY = event.rawY
                downTimeMs = System.currentTimeMillis()
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                val dx = (event.rawX - initialTouchX).toInt()
                val dy = (event.rawY - initialTouchY).toInt()
                layoutParams.x = initialX + dx
                layoutParams.y = initialY + dy
                runCatching { windowManager.updateViewLayout(view, layoutParams) }
                return true
            }

            MotionEvent.ACTION_UP -> {
                val movedX = abs(event.rawX - initialTouchX)
                val movedY = abs(event.rawY - initialTouchY)
                val elapsed = System.currentTimeMillis() - downTimeMs
                val isTap = movedX < TAP_SLOP && movedY < TAP_SLOP && elapsed < TAP_TIMEOUT_MS

                if (isTap) {
                    onClick()
                } else {
                    onDragEnd?.invoke(layoutParams.x, layoutParams.y)
                }
                return true
            }
        }
        return false
    }

    companion object {
        private const val TAP_SLOP = 12  // px - ngưỡng coi là "không di chuyển"
        private const val TAP_TIMEOUT_MS = 200L
    }
}
