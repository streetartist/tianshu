// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'widget_instance.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

WidgetInstance _$WidgetInstanceFromJson(Map<String, dynamic> json) =>
    WidgetInstance(
      id: json['id'] as String,
      type: json['type'] as String,
      x: (json['x'] as num?)?.toDouble() ?? 0,
      y: (json['y'] as num?)?.toDouble() ?? 0,
      width: (json['width'] as num?)?.toDouble() ?? 100,
      height: (json['height'] as num?)?.toDouble() ?? 50,
      properties: json['properties'] as Map<String, dynamic>?,
      dataBinding: json['dataBinding'] == null
          ? null
          : DataBinding.fromJson(json['dataBinding'] as Map<String, dynamic>),
    );

Map<String, dynamic> _$WidgetInstanceToJson(WidgetInstance instance) =>
    <String, dynamic>{
      'id': instance.id,
      'type': instance.type,
      'x': instance.x,
      'y': instance.y,
      'width': instance.width,
      'height': instance.height,
      'properties': instance.properties,
      'dataBinding': instance.dataBinding,
    };
