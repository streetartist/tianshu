import 'package:json_annotation/json_annotation.dart';
import 'data_binding.dart';

part 'widget_instance.g.dart';

/// 控件实例模型
@JsonSerializable()
class WidgetInstance {
  final String id;
  String type;
  double x;
  double y;
  double width;
  double height;
  Map<String, dynamic> properties;
  DataBinding? dataBinding;

  WidgetInstance({
    required this.id,
    required this.type,
    this.x = 0,
    this.y = 0,
    this.width = 100,
    this.height = 50,
    Map<String, dynamic>? properties,
    this.dataBinding,
  }) : properties = properties ?? {};

  factory WidgetInstance.fromJson(Map<String, dynamic> json) =>
      _$WidgetInstanceFromJson(json);

  Map<String, dynamic> toJson() => _$WidgetInstanceToJson(this);
}
