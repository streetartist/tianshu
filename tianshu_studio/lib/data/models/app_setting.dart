/// 应用设置项
class AppSetting {
  final String key;
  String label;
  String defaultValue;
  bool visible; // 是否在设置页面显示给用户
  String inputType; // text, url, password, number

  AppSetting({
    required this.key,
    required this.label,
    this.defaultValue = '',
    this.visible = true,
    this.inputType = 'text',
  });

  Map<String, dynamic> toJson() => {
        'key': key,
        'label': label,
        'defaultValue': defaultValue,
        'visible': visible,
        'inputType': inputType,
      };

  factory AppSetting.fromJson(Map<String, dynamic> json) => AppSetting(
        key: json['key'] ?? '',
        label: json['label'] ?? '',
        defaultValue: json['defaultValue'] ?? '',
        visible: json['visible'] ?? true,
        inputType: json['inputType'] ?? 'text',
      );
}

/// 应用设置配置
class AppSettingsConfig {
  List<AppSetting> settings;

  AppSettingsConfig({List<AppSetting>? settings})
      : settings = settings ?? _defaultSettings();

  static List<AppSetting> _defaultSettings() => [
        AppSetting(
          key: 'server_url',
          label: '天枢IoT平台api地址',
          defaultValue: 'https://tianshuapi.streetartist.top',
          visible: false, // 默认不显示给用户
          inputType: 'url',
        ),
      ];
}
