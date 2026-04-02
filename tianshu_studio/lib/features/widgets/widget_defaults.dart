import 'widget_type.dart';

/// 控件默认配置
class WidgetDefaults {
  static Map<String, dynamic> getDefaultProperties(WidgetType type) {
    switch (type) {
      case WidgetType.numberDisplay:
        return {'value': 0, 'unit': '', 'precision': 1, 'fontSize': 24.0};
      case WidgetType.textDisplay:
        return {'text': '文本', 'fontSize': 16.0, 'color': '#000000'};
      case WidgetType.statusLight:
        return {'status': false, 'onColor': '#4CAF50', 'offColor': '#9E9E9E'};
      case WidgetType.progressBar:
        return {'value': 50, 'min': 0, 'max': 100, 'color': '#2196F3'};
      case WidgetType.gauge:
        return {'value': 50, 'min': 0, 'max': 100, 'unit': ''};
      case WidgetType.numberInput:
        return {'value': 0, 'min': 0, 'max': 100, 'step': 1};
      case WidgetType.slider:
        return {'value': 50, 'min': 0, 'max': 100};
      case WidgetType.dropdown:
        return {'value': '', 'options': <String>[]};
      case WidgetType.switchButton:
        return {'value': false, 'label': '开关'};
      case WidgetType.button:
        return {'text': '按钮', 'command': ''};
      case WidgetType.lineChart:
        return {'title': '折线图', 'dataPoints': 20};
      case WidgetType.barChart:
        return {'title': '柱状图'};
      case WidgetType.container:
        return {'title': '容器', 'showBorder': true};
    }
  }

  static Size getDefaultSize(WidgetType type) {
    switch (type) {
      case WidgetType.gauge:
        return const Size(150, 150);
      case WidgetType.lineChart:
      case WidgetType.barChart:
        return const Size(300, 200);
      case WidgetType.container:
        return const Size(200, 150);
      default:
        return const Size(120, 50);
    }
  }
}

class Size {
  final double width;
  final double height;
  const Size(this.width, this.height);
}
