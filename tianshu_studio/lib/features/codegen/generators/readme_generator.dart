import '../models/generated_project.dart';

/// README生成器
class ReadmeGenerator {
  static GeneratedFile generate(String projectName) {
    final packageName = _toPackageName(projectName);

    return GeneratedFile(
      path: 'README.md',
      content: '''# $projectName

由天枢IoT Studio生成的Flutter项目

## 初始化项目

```bash
flutter create . --org com.example --project-name $packageName
flutter pub get
```

## 运行

```bash
flutter run
```

## 构建

```bash
flutter build apk      # Android
flutter build ios      # iOS
flutter build web      # Web
flutter build windows  # Windows
```
''',
    );
  }

  static String _toPackageName(String name) {
    if (RegExp(r'[\u4e00-\u9fa5]').hasMatch(name)) {
      return 'tianshu_app';
    }
    return name
        .toLowerCase()
        .replaceAll(RegExp(r'[^a-z0-9]'), '_')
        .replaceAll(RegExp(r'_+'), '_')
        .replaceAll(RegExp(r'^_|_$'), '');
  }
}
