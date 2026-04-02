// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'project_settings.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

ProjectSettings _$ProjectSettingsFromJson(Map<String, dynamic> json) =>
    ProjectSettings(
      serverUrl: json['serverUrl'] as String? ?? 'http://localhost:5000',
      timeout: (json['timeout'] as num?)?.toInt() ?? 30000,
      refreshInterval: (json['refreshInterval'] as num?)?.toInt() ?? 5000,
      enableServerSettings: json['enableServerSettings'] as bool? ?? true,
      enableUserLogin: json['enableUserLogin'] as bool? ?? false,
      enableLocalAiConfig: json['enableLocalAiConfig'] as bool? ?? true,
      enablePlatformAiConfig: json['enablePlatformAiConfig'] as bool? ?? false,
      enableThemeSettings: json['enableThemeSettings'] as bool? ?? true,
      enableLanguageSettings: json['enableLanguageSettings'] as bool? ?? true,
    );

Map<String, dynamic> _$ProjectSettingsToJson(ProjectSettings instance) =>
    <String, dynamic>{
      'serverUrl': instance.serverUrl,
      'timeout': instance.timeout,
      'refreshInterval': instance.refreshInterval,
      'enableServerSettings': instance.enableServerSettings,
      'enableUserLogin': instance.enableUserLogin,
      'enableLocalAiConfig': instance.enableLocalAiConfig,
      'enablePlatformAiConfig': instance.enablePlatformAiConfig,
      'enableThemeSettings': instance.enableThemeSettings,
      'enableLanguageSettings': instance.enableLanguageSettings,
    };
