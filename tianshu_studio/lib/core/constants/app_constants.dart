/// 应用常量定义
class AppConstants {
  AppConstants._();

  // 应用信息
  static const String appName = '天枢IoT Studio';
  static const String appVersion = '1.0.0';

  // API默认配置
  static const String defaultServerUrl = 'http://localhost:5000';
  static const int defaultTimeout = 30000;
  static const int defaultRefreshInterval = 5000;

  // 画布配置
  static const double defaultGridSize = 8.0;
  static const double minWidgetWidth = 50.0;
  static const double minWidgetHeight = 30.0;

  // 手机模拟器尺寸
  static const double phonePreviewWidth = 375.0;
  static const double phonePreviewHeight = 812.0;
}
