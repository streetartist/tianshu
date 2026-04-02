import '../../../data/models/layout_models.dart';
import '../../../data/models/device_slot.dart';
import '../../../data/models/app_setting.dart';

/// 布局画布状态
class LayoutCanvasState {
  final PageLayout page;
  final String? selectedItemId;
  final ProjectDeviceConfig deviceConfig;
  final AppSettingsConfig appSettings;

  LayoutCanvasState({
    required this.page,
    this.selectedItemId,
    ProjectDeviceConfig? deviceConfig,
    AppSettingsConfig? appSettings,
  }) : deviceConfig = deviceConfig ?? ProjectDeviceConfig(),
       appSettings = appSettings ?? AppSettingsConfig();

  LayoutCanvasState copyWith({
    PageLayout? page,
    String? selectedItemId,
    bool clearSelection = false,
    ProjectDeviceConfig? deviceConfig,
    AppSettingsConfig? appSettings,
  }) {
    return LayoutCanvasState(
      page: page ?? this.page,
      selectedItemId: clearSelection ? null : (selectedItemId ?? this.selectedItemId),
      deviceConfig: deviceConfig ?? this.deviceConfig,
      appSettings: appSettings ?? this.appSettings,
    );
  }
}
