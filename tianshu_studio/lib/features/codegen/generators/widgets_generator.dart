import '../models/generated_project.dart';
import '../templates/widgets_template.dart';

/// 控件代码生成器
class WidgetsGenerator {
  static GeneratedFile generate() {
    return GeneratedFile(
      path: 'lib/widgets/widgets.dart',
      content: WidgetsTemplate.generate(),
    );
  }
}
