import '../../../data/models/models.dart';

/// 画布状态
class CanvasState {
  final List<WidgetInstance> widgets;
  final String? selectedWidgetId;

  const CanvasState({
    this.widgets = const [],
    this.selectedWidgetId,
  });

  CanvasState copyWith({
    List<WidgetInstance>? widgets,
    String? selectedWidgetId,
    bool clearSelection = false,
  }) {
    return CanvasState(
      widgets: widgets ?? this.widgets,
      selectedWidgetId: clearSelection ? null : (selectedWidgetId ?? this.selectedWidgetId),
    );
  }
}
