// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'canvas_page.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

CanvasPage _$CanvasPageFromJson(Map<String, dynamic> json) => CanvasPage(
      id: json['id'] as String,
      name: json['name'] as String,
      widgets: (json['widgets'] as List<dynamic>?)
          ?.map((e) => WidgetInstance.fromJson(e as Map<String, dynamic>))
          .toList(),
      sortOrder: (json['sortOrder'] as num?)?.toInt() ?? 0,
    );

Map<String, dynamic> _$CanvasPageToJson(CanvasPage instance) =>
    <String, dynamic>{
      'id': instance.id,
      'name': instance.name,
      'widgets': instance.widgets,
      'sortOrder': instance.sortOrder,
    };
