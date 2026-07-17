package com.example.floatingoverlay.ui

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.ViewModelProvider
import com.example.floatingoverlay.databinding.ActivityMainBinding
import com.example.floatingoverlay.model.ServiceState
import com.example.floatingoverlay.util.PermissionUtils
import com.example.floatingoverlay.viewmodel.MainViewModel

/**
 * Activity duy nhất chịu trách nhiệm xin 3 quyền cần thiết (Overlay, Accessibility,
 * Notification) rồi bật/tắt [com.example.floatingoverlay.service.OverlayForegroundService].
 * Toàn bộ business logic nằm ở [MainViewModel] — Activity chỉ bind UI <-> LiveData.
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var viewModel: MainViewModel

    // Launcher xin quyền Overlay (mở màn hình Settings hệ thống, không có callback trực tiếp
    // nên ta kiểm tra lại trạng thái ở onResume/onActivityResult).
    private val overlayPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            viewModel.refreshPermissionState()
        }

    private val accessibilitySettingsLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            viewModel.refreshPermissionState()
        }

    // Runtime permission chuẩn cho POST_NOTIFICATIONS (Android 13+)
    private val notificationPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            viewModel.refreshPermissionState()
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        viewModel = ViewModelProvider(this)[MainViewModel::class.java]

        setupClickListeners()
        observeViewModel()
    }

    override fun onResume() {
        super.onResume()
        // Người dùng có thể vừa quay lại từ màn hình Cài đặt hệ thống -> luôn refresh.
        viewModel.refreshPermissionState()
    }

    private fun setupClickListeners() {
        binding.btnGrantOverlay.setOnClickListener {
            overlayPermissionLauncher.launch(PermissionUtils.overlayPermissionIntent(this))
        }

        binding.btnGrantAccessibility.setOnClickListener {
            accessibilitySettingsLauncher.launch(PermissionUtils.accessibilitySettingsIntent())
        }

        binding.btnGrantNotification.setOnClickListener {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                notificationPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
            }
        }

        binding.btnToggleService.setOnClickListener {
            viewModel.toggleService()
        }
    }

    private fun observeViewModel() {
        viewModel.overlayGranted.observe(this) { granted ->
            updatePermissionRow(binding.btnGrantOverlay, granted)
            refreshOverallStatus()
        }
        viewModel.accessibilityGranted.observe(this) { granted ->
            updatePermissionRow(binding.btnGrantAccessibility, granted)
            refreshOverallStatus()
        }
        viewModel.notificationGranted.observe(this) { granted ->
            updatePermissionRow(binding.btnGrantNotification, granted)
            // Card notification chỉ thực sự cần thiết & hiển thị trên Android 13+
            binding.cardNotification.visibility =
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    android.view.View.VISIBLE
                } else {
                    android.view.View.GONE
                }
            refreshOverallStatus()
        }
        viewModel.serviceState.observe(this) { state ->
            binding.btnToggleService.text = getString(
                if (state == ServiceState.RUNNING) {
                    com.example.floatingoverlay.R.string.btn_stop_service
                } else {
                    com.example.floatingoverlay.R.string.btn_start_service
                }
            )
        }
    }

    private fun updatePermissionRow(
        button: com.google.android.material.button.MaterialButton,
        granted: Boolean
    ) {
        button.isEnabled = !granted
        button.text = getString(
            if (granted) com.example.floatingoverlay.R.string.btn_granted
            else com.example.floatingoverlay.R.string.btn_grant
        )
    }

    private fun refreshOverallStatus() {
        val allGranted = viewModel.allPermissionsGranted()
        binding.btnToggleService.isEnabled = allGranted
        binding.tvStatus.text = getString(
            if (allGranted) com.example.floatingoverlay.R.string.status_all_granted
            else com.example.floatingoverlay.R.string.status_missing_permission
        )
    }
}
