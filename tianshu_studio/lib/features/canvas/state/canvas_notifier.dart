import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:uuid/uuid.dart';
import '../../../data/models/models.dart';
import '../../widgets/widget_type.dart';
import '../../widgets/widget_defaults.dart';
import 'canvas_state.dart';

/// 画布状态管理器
class CanvasNotifier extends StateNotifier<CanvasState> {
  CanvasNotifier() : super(const CanvasState());

  final _uuid = const Uuid();

  /// 添加控件
  void addWidget(WidgetType type, double x, double y) {
    final defaults = WidgetDefaults.getDefaultSize(type);
    final widget = WidgetInstance(
      id: _uuid.v4(),
      type: type.name,
      x: x,
      y: y,
      width: defaults.width,
      height: defaults.height,
      properties: WidgetDefaults.getDefaultProperties(type),
    );
    state = state.copyWith(
      widgets: [...state.widgets, widget],
      selectedWidgetId: widget.id,
    );
  }

  /// 选中控件
  void selectWidget(String? id) {
    state = state.copyWith(selectedWidgetId: id, clearSelection: id == null);
  }

  /// 移动控件
  void moveWidget(String id, double x, double y) {
    final widgets = state.widgets.map((w) {
      if (w.id == id) {
        w.x = x;
        w.y = y;
      }
      return w;
    }).toList();
    state = state.copyWith(widgets: widgets);
  }

  /// 删除控件
  void deleteWidget(String id) {
    final widgets = state.widgets.where((w) => w.id != id).toList();
    state = state.copyWith(widgets: widgets, clearSelection: true);
  }

  /// 更新控件属性
  void updateWidgetProperty(String id, String key, dynamic value) {
    final widgets = state.widgets.map((w) {
      if (w.id == id) {
        w.properties[key] = value;
      }
      return w;
    }).toList();
    state = state.copyWith(widgets: widgets);
  }
}

/// 画布状态Provider
final canvasProvider = StateNotifierProvider<CanvasNotifier, CanvasState>(
  (ref) => CanvasNotifier(),
);
