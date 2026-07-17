package com.example.floatingoverlay.service

import android.content.Context
import android.graphics.PixelFormat
import android.os.Build
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.WindowManager
import com.example.floatingoverlay.model.OverlaySettings
import com.example.floatingoverlay.util.DraggableTouchListener

/**
 * Chịu trách nhiệm duy nhất: quản lý vòng đời 2 view overlay (bubble nổi + panel điều khiển)
 * thông qua WindowManager. Không chứa business logic Start/Stop/Settings — các callback
 * được truyền từ ngoài vào (Dependency Injection thủ công) để giữ class này thuần về UI.
 *
 * Tách riêng khỏi Service để có thể unit test dễ dàng hơn (dù WindowManager thật
 * chỉ hoạt động đúng trong môi trường có display).
 */
class FloatingBubbleController(
    private val context: Context,
    private val onBubbleClick: () -> Unit,
    private val onStartClick: () -> Unit,
    private val onStopClick: () -> Unit,
    private val onSettingsClick: () -> Unit
) {

    private val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    private val inflater = LayoutInflater.from(context)

    private var bubbleView: View? = null
    private var bubbleParams: WindowManager.LayoutParams? = null

    private var panelView: View? = null
    private var panelParams: WindowManager.LayoutParams? = null

    private var isPanelVisible = false

    /** Loại overlay window: khác nhau giữa Android <8 và >=8 (Oreo). */
    private val overlayWindowType: Int =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
        } else {
            @Suppress("DEPRECATION")
            WindowManager.LayoutParams.TYPE_PHONE
        }

    // ---------- BUBBLE (nút nổi tròn, kéo thả được) ----------

    fun showBubble(settings: OverlaySettings) {
        if (bubbleView != null) return // đã hiển thị rồi, tránh add trùng

        val view = inflater.inflate(
            com.example.floatingoverlay.R.layout.view_floating_bubble, null
        )
        val sizePx = dpToPx(settings.buttonSizeDp)

        val params = WindowManager.LayoutParams(
            sizePx,
            sizePx,
            overlayWindowType,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            x = 0
            y = 300
        }

        view.alpha = settings.alphaFloat()
        view.setOnTouchListener(
            DraggableTouchListener(
                windowManager = windowManager,
                layoutParams = params,
                onClick = { onBubbleClick() }
            )
        )

        windowManager.addView(view, params)
        bubbleView = view
        bubbleParams = params
    }

    fun updateBubbleAppearance(settings: OverlaySettings) {
        val view = bubbleView ?: return
        val params = bubbleParams ?: return
        view.alpha = settings.alphaFloat()
        val sizePx = dpToPx(settings.buttonSizeDp)
        params.width = sizePx
        params.height = sizePx
        runCatching { windowManager.updateViewLayout(view, params) }
    }

    fun hideBubble() {
        bubbleView?.let { runCatching { windowManager.removeView(it) } }
        bubbleView = null
        bubbleParams = null
    }

    // ---------- PANEL (bảng điều khiển Start/Stop/Settings) ----------

    fun togglePanel() {
        if (isPanelVisible) hidePanel() else showPanel()
    }

    private fun showPanel() {
        if (panelView != null) return

        val view = inflater.inflate(
            com.example.floatingoverlay.R.layout.view_control_panel, null
        )

        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            overlayWindowType,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            // Hiện panel gần vị trí bubble hiện tại, có giới hạn để không tràn màn hình
            x = (bubbleParams?.x ?: 0).coerceAtMost(400)
            y = (bubbleParams?.y ?: 300) + dpToPx(64)
        }

        view.findViewById<View>(com.example.floatingoverlay.R.id.btnPanelStart)
            .setOnClickListener { onStartClick() }
        view.findViewById<View>(com.example.floatingoverlay.R.id.btnPanelStop)
            .setOnClickListener { onStopClick() }
        view.findViewById<View>(com.example.floatingoverlay.R.id.btnPanelSettings)
            .setOnClickListener { onSettingsClick() }
        view.findViewById<View>(com.example.floatingoverlay.R.id.ivClosePanel)
            .setOnClickListener { hidePanel() }

        windowManager.addView(view, params)
        panelView = view
        panelParams = params
        isPanelVisible = true
    }

    /** Cập nhật text trạng thái Running/Stopped hiển thị trong panel. */
    fun updatePanelStatus(text: String) {
        panelView
            ?.findViewById<android.widget.TextView>(com.example.floatingoverlay.R.id.tvPanelStatus)
            ?.text = text
    }

    fun hidePanel() {
        panelView?.let { runCatching { windowManager.removeView(it) } }
        panelView = null
        panelParams = null
        isPanelVisible = false
    }

    /** Dọn dẹp toàn bộ view khi Service bị destroy — BẮT BUỘC gọi để tránh leak WindowManager. */
    fun release() {
        hideBubble()
        hidePanel()
    }

    private fun dpToPx(dp: Int): Int =
        (dp * context.resources.displayMetrics.density).toInt()
}
