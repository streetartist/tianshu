import 'package:json_annotation/json_annotation.dart';
import 'canvas_page.dart';
import 'project_settings.dart';

part 'project.g.dart';

/// 项目模型
@JsonSerializable()
class Project {
  final String id;
  String name;
  String? description;
  List<CanvasPage> pages;
  ProjectSettings settings;
  DateTime createdAt;
  DateTime updatedAt;

  Project({
    required this.id,
    required this.name,
    this.description,
    List<CanvasPage>? pages,
    ProjectSettings? settings,
    DateTime? createdAt,
    DateTime? updatedAt,
  })  : pages = pages ?? [],
        settings = settings ?? ProjectSettings(),
        createdAt = createdAt ?? DateTime.now(),
        updatedAt = updatedAt ?? DateTime.now();

  factory Project.fromJson(Map<String, dynamic> json) =>
      _$ProjectFromJson(json);

  Map<String, dynamic> toJson() => _$ProjectToJson(this);
}
