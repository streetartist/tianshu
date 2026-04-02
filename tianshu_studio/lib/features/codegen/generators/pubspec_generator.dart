import '../../../data/models/models.dart';
import '../models/generated_project.dart';

/// pubspec.yaml生成器
class PubspecGenerator {
  static GeneratedFile generate(Project project) {
    final packageName = _toValidPackageName(project.name);
    return GeneratedFile(
      path: 'pubspec.yaml',
      content: '''name: $packageName
description: 由天枢IoT Studio生成

publish_to: 'none'
version: 1.0.0+1

environment:
  sdk: ^3.0.0

dependencies:
  flutter:
    sdk: flutter
  http: ^1.2.0
  shared_preferences: ^2.2.2
  socket_io_client: ^2.0.3+1

dev_dependencies:
  flutter_test:
    sdk: flutter
  flutter_lints: ^3.0.0

flutter:
  uses-material-design: true
''',
    );
  }

  static String _toValidPackageName(String s) {
    // 如果包含中文或非法字符，使用默认名称
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(s)) {
      return 'tianshu_app';
    }
    // 转换为小写下划线格式，只保留字母数字下划线
    final result = s
        .toLowerCase()
        .replaceAll(' ', '_')
        .replaceAll(RegExp(r'[^a-z0-9_]'), '');
    // 确保以字母开头
    if (result.isEmpty || !RegExp(r'^[a-z]').hasMatch(result)) {
      return 'tianshu_app';
    }
    return result;
  }
}
