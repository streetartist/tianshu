import '../../data/models/models.dart';
import 'models/generated_project.dart';
import 'generators/main_generator.dart';
import 'generators/app_generator.dart';
import 'generators/page_generator.dart';
import 'generators/service_generator.dart';
import 'generators/pubspec_generator.dart';
import 'generators/widgets_generator.dart';

/// 代码生成引擎
class CodeGenerator {
  final Project project;

  CodeGenerator(this.project);

  /// 生成完整项目
  GeneratedProject generate() {
    final files = <GeneratedFile>[];

    // 生成入口文件
    files.add(MainGenerator.generate());

    // 生成App配置
    files.add(AppGenerator.generate(project));

    // 生成页面
    files.addAll(PageGenerator.generate(project));

    // 生成服务层
    files.addAll(ServiceGenerator.generate(project));

    // 生成控件库
    files.add(WidgetsGenerator.generate());

    // 生成pubspec.yaml
    files.add(PubspecGenerator.generate(project));

    // 生成README
    files.add(_generateReadme());

    return GeneratedProject(name: project.name, files: files);
  }

  GeneratedFile _generateReadme() {
    return GeneratedFile(
      path: 'README.md',
      content: '''# ${project.name}

由天枢IoT Studio生成的Flutter应用。

## 运行

```bash
flutter pub get
flutter run
```
''',
    );
  }
}
