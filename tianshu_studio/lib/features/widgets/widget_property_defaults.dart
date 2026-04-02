import 'widget_type.dart';

/// 控件默认属性（包含数据绑定）
class WidgetPropertyDefaults {
  static Map<String, dynamic> getDefaultProperties(WidgetType type) {
    switch (type) {
      case WidgetType.numberDisplay:
        return {
          'label': '温度',
          'unit': '°C',
          'precision': 1,
          // 数据绑定 - slotId为'default'表示跟随页面/全局设备配置
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.textDisplay:
        return {
          'label': '状态',
          'text': '--',
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.statusLight:
        return {
          'label': '在线状态',
          'onColor': '#4CAF50',
          'offColor': '#9E9E9E',
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.progressBar:
        return {
          'label': '电量',
          'min': 0,
          'max': 100,
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.gauge:
        return {
          'label': '湿度',
          'min': 0,
          'max': 100,
          'unit': '%',
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.switchButton:
        return {
          'label': '开关',
          // 控制绑定
          'slotId': 'default',
          'command': '',  // 如: power
          'onValue': 'on',
          'offValue': 'off',
        };

      case WidgetType.button:
        return {
          'text': '执行',
          'slotId': 'default',
          'command': '',
          'commandValue': '',      // 发送的值（无参数时为空）
          'commandType': 'none',   // 值类型: none/bool/number/string/json
        };

      case WidgetType.slider:
        return {
          'label': '亮度',
          'min': 0,
          'max': 100,
          'slotId': 'default',
          'command': '',
        };

      case WidgetType.numberInput:
        return {
          'label': '设定值',
          'min': 0,
          'max': 100,
          'step': 1,
          'slotId': 'default',
          'command': '',
        };

      case WidgetType.dropdown:
        return {
          'label': '模式',
          'options': '自动,制冷,制热',
          'slotId': 'default',
          'command': '',
        };

      case WidgetType.lineChart:
        return {
          'title': '温度趋势',
          'slotId': 'default',
          'fieldName': '',
          'timeRange': 24,  // 小时
        };

      case WidgetType.barChart:
        return {
          'title': '数据统计',
          'slotId': 'default',
          'fieldName': '',
        };

      case WidgetType.container:
        return {
          'title': '分组',
          'showBorder': true,
          'showTitle': true,
          'layout': 'column',  // column, row, wrap, grid
          'gridColumns': 2,
        };
    }
  }
}
