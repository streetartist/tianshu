// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'data_binding.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

DataBinding _$DataBindingFromJson(Map<String, dynamic> json) => DataBinding(
      deviceId: json['deviceId'] as String,
      fieldName: json['fieldName'] as String,
      expression: json['expression'] as String?,
    );

Map<String, dynamic> _$DataBindingToJson(DataBinding instance) =>
    <String, dynamic>{
      'deviceId': instance.deviceId,
      'fieldName': instance.fieldName,
      'expression': instance.expression,
    };
