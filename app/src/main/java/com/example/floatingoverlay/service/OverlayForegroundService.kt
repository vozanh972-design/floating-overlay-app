package com.example.floatingoverlay.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.example.floatingoverlay.R
import com.example.floatingoverlay.model.OverlaySettings
import com.example.floatingoverlay.model.SettingsRepository
import com.example.floatingoverlay.ui.MainActivity
import com.example.floatingoverlay.ui.SettingsActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * Foreground Service — "nhạc trưởng" điều phối:
 *   1. Hiển thị bubble nổi ngay khi service start.
 *   2. Lắng nghe thay đổi Settings (DataStore) realtime để cập nhật giao diện bubble.
 *   3. Nhận lệnh Start/Stop/Settings từ panel và phản hồi lại qua [FloatingBubbleController].
 *
 * Dùng Foreground Service (thay vì Service thường) vì:
 *   - Android 8+ giới hạn nghiêm ngặt background services.
 *   - Overlay cần tồn tại lâu dài, độc lập với vòng đời Activity → bắt buộc phải là FGS
 *     với loại "specialUse" (khai báo rõ trong Manifest) trên Android 14+.
 */
class OverlayForegroundService : Service() {

    private val serviceJob = SupervisorJob()
    private val serviceScope = CoroutineScope(serviceJob)

    private lateinit var controller: FloatingBubbleController
    private lateinit var settingsRepository: SettingsRepository

    // Trạng thái "automation đang chạy hay không" — độc lập với việc bubble có hiển thị.
    // Đây là nơi bạn tự cắm logic Start/Stop thực tế theo mục đích hợp lệ của ứng dụng.
    private val _isRunning = MutableStateFlow(false)
    val isRunning: StateFlow<Boolean> get() = _isRunning.asStateFlow()

    override fun onCreate() {
        super.onCreate()
        settingsRepository = SettingsRepository.getInstance(applicationContext)

        controller = FloatingBubbleController(
            context = applicationContext,
            onBubbleClick = { controller.togglePanel() },
            onStartClick = { handleStart() },
            onStopClick = { handleStop() },
            onSettingsClick = { openSettingsActivity() }
        )

        createNotificationChannelIfNeeded()
        startForeground(NOTIFICATION_ID, buildNotification())

        // Hiển thị bubble với cấu hình đã lưu, và tiếp tục lắng nghe thay đổi từ Settings.
        serviceScope.launch {
            settingsRepository.settingsFlow.collect { settings ->
                onSettingsChanged(settings)
            }
        }
    }

    private var firstSettingsEmit = true
    private fun onSettingsChanged(settings: OverlaySettings) {
        if (firstSettingsEmit) {
            controller.showBubble(settings)
            firstSettingsEmit = false
        } else {
            controller.updateBubbleAppearance(settings)
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                stopSelf()
            }
        }
        // START_STICKY: hệ thống sẽ cố khởi động lại service nếu bị kill do thiếu bộ nhớ,
        // phù hợp với overlay cần tồn tại liên tục.
        return START_STICKY
    }

    private fun handleStart() {
        _isRunning.value = true
        controller.updatePanelStatus(getString(R.string.service_running))
        // TODO: Kích hoạt logic automation/tác vụ thực tế của bạn tại đây.
    }

    private fun handleStop() {
        _isRunning.value = false
        controller.updatePanelStatus(getString(R.string.service_stopped))
        // TODO: Dừng logic automation/tác vụ thực tế của bạn tại đây.
    }

    private fun openSettingsActivity() {
        controller.hidePanel()
        val intent = Intent(this, SettingsActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        startActivity(intent)
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        controller.release()
        serviceScope.cancel()
        super.onDestroy()
    }

    // ---------- Notification (bắt buộc cho Foreground Service) ----------

    private fun createNotificationChannelIfNeeded() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val manager = getSystemService(NotificationManager::class.java)
            val channel = NotificationChannel(
                CHANNEL_ID,
                getString(R.string.notification_channel_name),
                NotificationManager.IMPORTANCE_MIN // MIN để không làm phiền bằng âm thanh/rung
            ).apply {
                description = getString(R.string.notification_channel_desc)
            }
            manager.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(): Notification {
        val contentIntent = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(getString(R.string.notification_title))
            .setContentText(getString(R.string.notification_content))
            .setSmallIcon(R.drawable.ic_fab_circle)
            .setPriority(NotificationCompat.PRIORITY_MIN)
            .setOngoing(true)
            .setContentIntent(contentIntent)
            .build()
    }

    companion object {
        private const val CHANNEL_ID = "overlay_service_channel"
        private const val NOTIFICATION_ID = 1001
        private const val ACTION_STOP = "com.example.floatingoverlay.action.STOP"

        fun start(context: Context) {
            val intent = Intent(context, OverlayForegroundService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(intent)
            } else {
                context.startService(intent)
            }
        }

        fun requestStop(context: Context) {
            context.stopService(Intent(context, OverlayForegroundService::class.java))
        }
    }
}
