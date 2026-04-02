import 'package:json_annotation/json_annotation.dart';

part 'data_binding.g.dart';

/// 数据绑定模型
@JsonSerializable()
class DataBinding {
  String deviceId;
  String fieldName;
  String? expression;

  DataBinding({
    required this.deviceId,
    required this.fieldName,
    this.expression,
  });

  factory DataBinding.fromJson(Map<String, dynamic> json) =>
      _$DataBindingFromJson(json);

  Map<String, dynamic> toJson() => _$DataBindingToJson(this);
}
