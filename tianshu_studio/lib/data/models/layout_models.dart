/// 布局类型
enum LayoutType {
  column('纵向排列'),
  row('横向排列'),
  grid('网格布局'),
  list('列表布局'),
  wrap('流式布局');

  final String label;
  const LayoutType(this.label);
}

/// 页面布局模型
class PageLayout {
  final String id;
  String name;
  LayoutType layoutType;
  String defaultDeviceId;
  List<LayoutItem> children;
  Map<String, dynamic> properties;

  PageLayout({
    required this.id,
    required this.name,
    this.layoutType = LayoutType.column,
    this.defaultDeviceId = '',
    List<LayoutItem>? children,
    Map<String, dynamic>? properties,
  })  : children = children ?? [],
        properties = properties ?? {};
}

/// 布局项（可以是控件或嵌套布局）
class LayoutItem {
  final String id;
  String type; // widget类型或layout
  int flex; // 弹性比例
  double? height; // 固定高度（可选）
  double? width; // 固定宽度（可选）
  EdgeInsetsData padding;
  EdgeInsetsData margin;
  Map<String, dynamic> properties;
  List<LayoutItem>? children; // 如果是容器则有子项

  LayoutItem({
    required this.id,
    required this.type,
    this.flex = 1,
    this.height,
    this.width,
    EdgeInsetsData? padding,
    EdgeInsetsData? margin,
    Map<String, dynamic>? properties,
    this.children,
  })  : padding = padding ?? EdgeInsetsData.zero,
        margin = margin ?? EdgeInsetsData.zero,
        properties = properties ?? {};

  bool get isContainer => children != null;
}

/// 边距数据
class EdgeInsetsData {
  final double left, top, right, bottom;

  const EdgeInsetsData({
    this.left = 0,
    this.top = 0,
    this.right = 0,
    this.bottom = 0,
  });

  static const zero = EdgeInsetsData();

  factory EdgeInsetsData.all(double value) => EdgeInsetsData(
        left: value,
        top: value,
        right: value,
        bottom: value,
      );
}
