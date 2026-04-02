import 'package:flutter/material.dart';
import 'package:flutter/foundation.dart' show kIsWeb, defaultTargetPlatform, TargetPlatform;

/// 应用主题配置
class AppTheme {
  AppTheme._();

  // 主色调
  static const Color primaryColor = Color(0xFF1976D2);
  static const Color secondaryColor = Color(0xFF26A69A);

  // 根据平台选择字体
  static String? get _fontFamily {
    if (!kIsWeb && defaultTargetPlatform == TargetPlatform.windows) {
      return 'Microsoft YaHei';  // 微软雅黑
    }
    return null;  // Web 和其他平台用默认字体
  }

  // 亮色主题
  static ThemeData get lightTheme => ThemeData(
        useMaterial3: true,
        fontFamily: _fontFamily,
        colorScheme: ColorScheme.fromSeed(
          seedColor: primaryColor,
          brightness: Brightness.light,
        ),
        appBarTheme: const AppBarTheme(
          centerTitle: true,
          elevation: 0,
        ),
      );

  // 暗色主题
  static ThemeData get darkTheme => ThemeData(
        useMaterial3: true,
        fontFamily: _fontFamily,
        colorScheme: ColorScheme.fromSeed(
          seedColor: primaryColor,
          brightness: Brightness.dark,
        ),
        appBarTheme: const AppBarTheme(
          centerTitle: true,
          elevation: 0,
        ),
      );
}
