import 'package:json_annotation/json_annotation.dart';
import 'widget_instance.dart';

part 'canvas_page.g.dart';

/// 画布页面模型
@JsonSerializable()
class CanvasPage {
  final String id;
  String name;
  List<WidgetInstance> widgets;
  int sortOrder;

  CanvasPage({
    required this.id,
    required this.name,
    List<WidgetInstance>? widgets,
    this.sortOrder = 0,
  }) : widgets = widgets ?? [];

  factory CanvasPage.fromJson(Map<String, dynamic> json) =>
      _$CanvasPageFromJson(json);

  Map<String, dynamic> toJson() => _$CanvasPageToJson(this);
}
