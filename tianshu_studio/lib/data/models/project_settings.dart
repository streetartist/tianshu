import 'package:json_annotation/json_annotation.dart';

part 'project_settings.g.dart';

/// 项目设置模型
@JsonSerializable()
class ProjectSettings {
  // 服务器配置
  String serverUrl;
  int timeout;
  int refreshInterval;

  // 生成App设置选项
  bool enableServerSettings;
  bool enableUserLogin;
  bool enableLocalAiConfig;
  bool enablePlatformAiConfig;
  bool enableThemeSettings;
  bool enableLanguageSettings;

  ProjectSettings({
    this.serverUrl = 'http://localhost:5000',
    this.timeout = 30000,
    this.refreshInterval = 5000,
    this.enableServerSettings = true,
    this.enableUserLogin = false,
    this.enableLocalAiConfig = true,
    this.enablePlatformAiConfig = false,
    this.enableThemeSettings = true,
    this.enableLanguageSettings = true,
  });

  factory ProjectSettings.fromJson(Map<String, dynamic> json) =>
      _$ProjectSettingsFromJson(json);

  Map<String, dynamic> toJson() => _$ProjectSettingsToJson(this);
}
