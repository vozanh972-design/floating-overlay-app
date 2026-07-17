package com.example.floatingoverlay.ui

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.ViewModelProvider
import com.example.floatingoverlay.OverlayApplication
import com.example.floatingoverlay.databinding.ActivitySettingsBinding
import com.example.floatingoverlay.viewmodel.SettingsViewModel

/**
 * Màn hình Settings mở ra khi người dùng nhấn nút "Settings" trên panel nổi.
 * Đọc/ghi cấu hình qua [SettingsViewModel], thay đổi được áp dụng NGAY LẬP TỨC
 * lên bubble đang hiển thị nhờ OverlayForegroundService lắng nghe Flow từ DataStore.
 */
class SettingsActivity : AppCompatActivity() {

    private lateinit var binding: ActivitySettingsBinding
    private lateinit var viewModel: SettingsViewModel

    private var isBindingUi = false // tránh vòng lặp listener khi set giá trị ban đầu

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val repository = (application as OverlayApplication).settingsRepository
        viewModel = ViewModelProvider(
            this,
            SettingsViewModel.Factory(repository)
        )[SettingsViewModel::class.java]

        observeSettings()
        setupListeners()
    }

    private fun observeSettings() {
        viewModel.settings.observe(this) { settings ->
            isBindingUi = true
            binding.sliderOpacity.value = settings.opacityPercent.toFloat()
            binding.sliderSize.value = settings.buttonSizeDp.toFloat()
            binding.switchVibration.isChecked = settings.vibrationEnabled
            binding.switchAutoStart.isChecked = settings.autoStartEnabled
            isBindingUi = false
        }
    }

    private fun setupListeners() {
        binding.sliderOpacity.addOnChangeListener { _, value, fromUser ->
            if (fromUser && !isBindingUi) viewModel.setOpacity(value.toInt())
        }
        binding.sliderSize.addOnChangeListener { _, value, fromUser ->
            if (fromUser && !isBindingUi) viewModel.setButtonSize(value.toInt())
        }
        binding.switchVibration.setOnCheckedChangeListener { _, isChecked ->
            if (!isBindingUi) viewModel.setVibration(isChecked)
        }
        binding.switchAutoStart.setOnCheckedChangeListener { _, isChecked ->
            if (!isBindingUi) viewModel.setAutoStart(isChecked)
        }
    }
}
