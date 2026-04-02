/// 设备槽位 - App中的设备占位符
class DeviceSlot {
  final String id;
  String name;
  String templateId;
  String defaultDeviceId; // 设备数据库ID
  String defaultSecretKey; // 设备密钥
  bool allowMultiple;

  DeviceSlot({
    required this.id,
    required this.name,
    this.templateId = '',
    this.defaultDeviceId = '',
    this.defaultSecretKey = '',
    this.allowMultiple = false,
  });
}

/// 项目设备配置
class ProjectDeviceConfig {
  List<DeviceSlot> slots;
  bool requirePairing; // 是否需要配对（产品级）
  bool enableDeviceManagement; // 是否启用设备管理页面（用户可在App中切换设备）

  ProjectDeviceConfig({
    List<DeviceSlot>? slots,
    this.requirePairing = false,
    this.enableDeviceManagement = false,
  }) : slots = slots ?? [
    DeviceSlot(id: 'default', name: '默认设备'),
  ];

  ProjectDeviceConfig copyWith({
    List<DeviceSlot>? slots,
    bool? requirePairing,
    bool? enableDeviceManagement,
  }) {
    return ProjectDeviceConfig(
      slots: slots ?? this.slots,
      requirePairing: requirePairing ?? this.requirePairing,
      enableDeviceManagement: enableDeviceManagement ?? this.enableDeviceManagement,
    );
  }
}
