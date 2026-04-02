import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../data/models/models.dart';
import '../../data/services/services.dart';

/// API服务Provider
final apiServiceProvider = Provider<ApiService>((ref) {
  return ApiService();
});

/// 设备服务Provider
final deviceServiceProvider = Provider<DeviceService>((ref) {
  return DeviceService(ref.watch(apiServiceProvider));
});

/// AI服务Provider
final aiServiceProvider = Provider<AiService>((ref) {
  return AiService(ref.watch(apiServiceProvider));
});

/// 当前项目Provider
final currentProjectProvider = StateProvider<Project?>((ref) => null);

/// 当前页面索引Provider
final currentPageIndexProvider = StateProvider<int>((ref) => 0);

/// 选中控件Provider
final selectedWidgetProvider = StateProvider<WidgetInstance?>((ref) => null);
