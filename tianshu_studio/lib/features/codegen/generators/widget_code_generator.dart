import '../../../data/models/models.dart';
import '../../../data/models/layout_models.dart';

/// 控件代码生成器
class WidgetCodeGenerator {
  /// 从LayoutItem列表生成控件代码（新布局系统）
  static String generateFromLayout(List<LayoutItem> items) {
    final buffer = StringBuffer();
    for (final item in items) {
      buffer.writeln('        ${_generateLayoutWidget(item)},');
    }
    return buffer.toString();
  }

  static String _generateLayoutWidget(LayoutItem item) {
    final p = item.properties;
    final slotId = p['slotId'] ?? 'default';
    final fieldName = p['fieldName'] ?? '';
    final command = p['command'] ?? '';

    switch (item.type) {
      case 'numberDisplay':
        return _genNumberDisplay(p, slotId, fieldName);
      case 'textDisplay':
        return _genTextDisplay(p, slotId, fieldName);
      case 'statusLight':
        return _genStatusLight(p, slotId, fieldName);
      case 'progressBar':
        return _genProgressBar(p, slotId, fieldName);
      case 'gauge':
        return _genGauge(p, slotId, fieldName);
      case 'switchButton':
        return _genSwitch(p, slotId, command);
      case 'button':
        return _genButton(p, slotId, command);
      case 'slider':
        return _genSlider(p, slotId, command);
      case 'dropdown':
        return _genDropdown(p, slotId, command);
      case 'numberInput':
        return _genNumberInput(p, slotId, command);
      case 'lineChart':
        return _genLineChart(p, slotId, fieldName);
      case 'barChart':
        return _genBarChart(p, slotId, fieldName);
      case 'container':
        return _genContainer(item);
      default:
        return "const SizedBox() // TODO: ${item.type}";
    }
  }

  static String _genNumberDisplay(Map<String, dynamic> p, String slot, String field) {
    return '''NumberDisplayWidget(
          label: '${p['label'] ?? ''}',
          unit: '${p['unit'] ?? ''}',
          value: iot.getValue('$slot', '$field'),
        )''';
  }

  static String _genTextDisplay(Map<String, dynamic> p, String slot, String field) {
    return '''TextDisplayWidget(
          label: '${p['label'] ?? ''}',
          text: iot.getString('$slot', '$field'),
        )''';
  }

  static String _genStatusLight(Map<String, dynamic> p, String slot, String field) {
    return '''StatusLightWidget(
          label: '${p['label'] ?? ''}',
          status: iot.getBool('$slot', '$field'),
        )''';
  }

  static String _genProgressBar(Map<String, dynamic> p, String slot, String field) {
    return '''ProgressBarWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getValue('$slot', '$field'),
          min: ${p['min'] ?? 0}.0,
          max: ${p['max'] ?? 100}.0,
        )''';
  }

  static String _genGauge(Map<String, dynamic> p, String slot, String field) {
    return '''GaugeWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getValue('$slot', '$field'),
          min: ${p['min'] ?? 0}.0,
          max: ${p['max'] ?? 100}.0,
          unit: '${p['unit'] ?? ''}',
        )''';
  }

  static String _genSwitch(Map<String, dynamic> p, String slot, String cmd) {
    return '''SwitchWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getBool('$slot', '$cmd'),
          onChanged: (v) => iot.send('$slot', '$cmd', v),
        )''';
  }

  static String _genButton(Map<String, dynamic> p, String slot, String cmd) {
    final valueStr = p['commandValue'] ?? '';
    final valueType = p['commandType'] ?? 'none';

    // 根据类型生成正确的值
    String valueCode;
    switch (valueType) {
      case 'none':
        valueCode = 'null';
        break;
      case 'bool':
        valueCode = valueStr == 'true' ? 'true' : 'false';
        break;
      case 'number':
        valueCode = num.tryParse(valueStr)?.toString() ?? '0';
        break;
      case 'json':
        valueCode = valueStr.isNotEmpty ? valueStr : '{}';
        break;
      case 'string':
      default:
        valueCode = valueStr.isNotEmpty ? "'$valueStr'" : "null";
    }

    return '''ButtonWidget(
          text: '${p['text'] ?? '按钮'}',
          onPressed: () => iot.send('$slot', '$cmd', $valueCode),
        )''';
  }

  static String _genSlider(Map<String, dynamic> p, String slot, String cmd) {
    return '''SliderWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getValue('$slot', '$cmd'),
          min: ${p['min'] ?? 0}.0,
          max: ${p['max'] ?? 100}.0,
          onChanged: (v) => iot.send('$slot', '$cmd', v),
        )''';
  }

  static String _genDropdown(Map<String, dynamic> p, String slot, String cmd) {
    final opts = (p['options'] ?? '').toString().split(',');
    final optList = opts.map((o) => "'${o.trim()}'").join(', ');
    return '''DropdownWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getString('$slot', '$cmd'),
          options: [$optList],
          onChanged: (v) => iot.send('$slot', '$cmd', v),
        )''';
  }

  static String _genNumberInput(Map<String, dynamic> p, String slot, String cmd) {
    return '''NumberInputWidget(
          label: '${p['label'] ?? ''}',
          value: iot.getValue('$slot', '$cmd'),
          min: ${p['min'] ?? 0}.0,
          max: ${p['max'] ?? 100}.0,
          step: ${p['step'] ?? 1}.0,
          onChanged: (v) => iot.send('$slot', '$cmd', v),
        )''';
  }

  static String _genLineChart(Map<String, dynamic> p, String slot, String field) {
    return '''LineChartWidget(
          title: '${p['title'] ?? '趋势图'}',
          data: iot.getHistory('$slot', '$field'),
        )''';
  }

  static String _genBarChart(Map<String, dynamic> p, String slot, String field) {
    return '''BarChartWidget(
          title: '${p['title'] ?? '统计图'}',
          data: iot.getStats('$slot', '$field'),
        )''';
  }

  static String _genContainer(LayoutItem item) {
    final p = item.properties;
    final layout = p['layout'] ?? 'column';
    final showTitle = p['showTitle'] ?? true;
    final title = p['title'] ?? '分组';

    // 生成子控件代码
    final childrenCode = StringBuffer();
    if (item.children != null && item.children!.isNotEmpty) {
      for (final child in item.children!) {
        childrenCode.writeln('            ${_generateLayoutWidget(child)},');
      }
    }

    return '''ContainerWidget(
          title: ${showTitle ? "'$title'" : "null"},
          layout: ContainerLayout.$layout,
          showBorder: ${p['showBorder'] ?? true},
          children: [
$childrenCode          ],
        )''';
  }

  // 保留旧方法兼容
  static String generateWidgets(List<WidgetInstance> widgets) {
    final buffer = StringBuffer();
    for (final w in widgets) {
      buffer.writeln(_generateWidget(w));
    }
    return buffer.toString();
  }

  static String _generateWidget(WidgetInstance w) {
    final content = _generateWidgetContent(w);
    return '''          Positioned(
            left: ${w.x.toStringAsFixed(1)},
            top: ${w.y.toStringAsFixed(1)},
            child: SizedBox(
              width: ${w.width.toStringAsFixed(1)},
              height: ${w.height.toStringAsFixed(1)},
              child: $content,
            ),
          ),''';
  }

  static String _generateWidgetContent(WidgetInstance w) {
    switch (w.type) {
      case 'numberDisplay':
        return "Text('${w.properties['value'] ?? 0}${w.properties['unit'] ?? ''}')";
      case 'textDisplay':
        return "Text('${w.properties['text'] ?? ''}')";
      case 'switchButton':
        return "Switch(value: ${w.properties['value'] ?? false}, onChanged: (v) {})";
      case 'button':
        return "ElevatedButton(onPressed: () {}, child: Text('${w.properties['text'] ?? '按钮'}'))";
      default:
        return "Container(child: Text('${w.type}'))";
    }
  }
}
