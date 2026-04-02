import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:uuid/uuid.dart';
import '../../../data/models/layout_models.dart';
import '../../../data/models/device_slot.dart';
import '../../../data/models/app_setting.dart';
import '../../widgets/widget_type.dart';
import '../../widgets/widget_property_defaults.dart';
import 'layout_canvas_state.dart';

/// 布局画布状态管理器
class LayoutCanvasNotifier extends StateNotifier<LayoutCanvasState> {
  LayoutCanvasNotifier()
      : super(LayoutCanvasState(
          page: PageLayout(id: const Uuid().v4(), name: '首页'),
        ));

  final _uuid = const Uuid();

  /// 添加控件到末尾
  void addItem(String typeName, {Map<String, dynamic>? props}) {
    final type = WidgetType.values.firstWhere(
      (t) => t.name == typeName,
      orElse: () => WidgetType.textDisplay,
    );
    final defaultProps = WidgetPropertyDefaults.getDefaultProperties(type);
    if (props != null) defaultProps.addAll(props);

    final item = LayoutItem(
      id: _uuid.v4(),
      type: typeName,
      properties: defaultProps,
    );
    final newChildren = [...state.page.children, item];
    state = state.copyWith(
      page: PageLayout(
        id: state.page.id,
        name: state.page.name,
        layoutType: state.page.layoutType,
        children: newChildren,
      ),
      selectedItemId: item.id,
    );
  }

  /// 选中项
  void selectItem(String? id) {
    state = state.copyWith(
      selectedItemId: id,
      clearSelection: id == null,
    );
  }

  /// 删除项（支持容器内控件）
  void deleteItem(String id) {
    final newChildren = _removeItem(state.page.children, id);
    state = state.copyWith(
      page: PageLayout(
        id: state.page.id,
        name: state.page.name,
        layoutType: state.page.layoutType,
        children: newChildren,
      ),
      clearSelection: true,
    );
  }

  /// 移动项（上移/下移）
  void moveItem(String id, int direction) {
    final children = [...state.page.children];
    final index = children.indexWhere((c) => c.id == id);
    if (index < 0) return;

    final newIndex = index + direction;
    if (newIndex < 0 || newIndex >= children.length) return;

    final item = children.removeAt(index);
    children.insert(newIndex, item);

    _updateChildren(children);
  }

  /// 拖拽重排序
  void reorderItem(int oldIndex, int newIndex) {
    final children = [...state.page.children];
    if (newIndex > oldIndex) newIndex--;
    final item = children.removeAt(oldIndex);
    children.insert(newIndex, item);
    _updateChildren(children);
  }

  void _updateChildren(List<LayoutItem> children) {
    state = state.copyWith(
      page: PageLayout(
        id: state.page.id,
        name: state.page.name,
        layoutType: state.page.layoutType,
        children: children,
      ),
    );
  }

  /// 设置页面布局类型
  void setLayoutType(LayoutType type) {
    state = state.copyWith(
      page: PageLayout(
        id: state.page.id,
        name: state.page.name,
        layoutType: type,
        children: state.page.children,
      ),
    );
  }

  /// 添加控件到容器
  void addItemToContainer(String containerId, String typeName) {
    final type = WidgetType.values.firstWhere(
      (t) => t.name == typeName,
      orElse: () => WidgetType.textDisplay,
    );
    final defaultProps = WidgetPropertyDefaults.getDefaultProperties(type);
    final newItem = LayoutItem(
      id: _uuid.v4(),
      type: typeName,
      properties: defaultProps,
    );

    final children = _addToContainer(state.page.children, containerId, newItem);
    _updateChildren(children);
    state = state.copyWith(selectedItemId: newItem.id);
  }

  List<LayoutItem> _addToContainer(List<LayoutItem> items, String containerId, LayoutItem newItem) {
    return items.map((item) {
      if (item.id == containerId && item.type == 'container') {
        final List<LayoutItem> newChildren = [...(item.children ?? <LayoutItem>[]), newItem];
        return LayoutItem(
          id: item.id,
          type: item.type,
          properties: item.properties,
          children: newChildren,
        );
      } else if (item.children != null) {
        return LayoutItem(
          id: item.id,
          type: item.type,
          properties: item.properties,
          children: _addToContainer(item.children!, containerId, newItem),
        );
      }
      return item;
    }).toList();
  }

  /// 移动控件到容器
  void moveItemToContainer(String containerId, String itemId) {
    final itemToMove = _findItem(state.page.children, itemId);
    if (itemToMove == null) return;

    var children = _removeItem(state.page.children, itemId);
    children = _addToContainer(children, containerId, itemToMove);
    _updateChildren(children);
  }

  /// 移动控件到页面根级别
  void moveItemToRoot(String itemId) {
    final itemToMove = _findItem(state.page.children, itemId);
    if (itemToMove == null) return;

    // 检查是否已在根级别
    if (state.page.children.any((c) => c.id == itemId)) return;

    var children = _removeItem(state.page.children, itemId);
    children = [...children, itemToMove];
    _updateChildren(children);
  }

  LayoutItem? _findItem(List<LayoutItem> items, String id) {
    for (final item in items) {
      if (item.id == id) return item;
      if (item.children != null) {
        final found = _findItem(item.children!, id);
        if (found != null) return found;
      }
    }
    return null;
  }

  List<LayoutItem> _removeItem(List<LayoutItem> items, String id) {
    return items.where((item) => item.id != id).map((item) {
      if (item.children != null) {
        return LayoutItem(
          id: item.id,
          type: item.type,
          properties: item.properties,
          children: _removeItem(item.children!, id),
        );
      }
      return item;
    }).toList();
  }

  /// 更新项属性（支持容器内控件）
  void updateItemProperty(String id, String key, dynamic value) {
    final children = _updatePropertyRecursive(state.page.children, id, key, value);
    _updateChildren(children);
  }

  List<LayoutItem> _updatePropertyRecursive(
    List<LayoutItem> items, String id, String key, dynamic value,
  ) {
    return items.map((item) {
      if (item.id == id) {
        item.properties[key] = value;
        return item;
      }
      if (item.children != null) {
        return LayoutItem(
          id: item.id,
          type: item.type,
          properties: item.properties,
          children: _updatePropertyRecursive(item.children!, id, key, value),
        );
      }
      return item;
    }).toList();
  }

  /// 添加设备槽位
  void addSlot(String name, {String templateId = '', bool allowMultiple = false}) {
    final newSlot = DeviceSlot(
      id: _uuid.v4(),
      name: name,
      templateId: templateId,
      allowMultiple: allowMultiple,
    );
    final newSlots = [...state.deviceConfig.slots, newSlot];
    state = state.copyWith(
      deviceConfig: ProjectDeviceConfig(
        slots: newSlots,
        requirePairing: state.deviceConfig.requirePairing,
      ),
    );
  }

  /// 删除设备槽位
  void removeSlot(String slotId) {
    if (slotId == 'default') return;
    final newSlots = state.deviceConfig.slots
        .where((s) => s.id != slotId)
        .toList();
    state = state.copyWith(
      deviceConfig: ProjectDeviceConfig(
        slots: newSlots,
        requirePairing: state.deviceConfig.requirePairing,
      ),
    );
  }

  /// 更新槽位的默认设备ID
  void updateSlotDeviceId(String slotId, String deviceId) {
    final newSlots = state.deviceConfig.slots.map((s) {
      if (s.id == slotId) {
        s.defaultDeviceId = deviceId;
      }
      return s;
    }).toList();
    state = state.copyWith(
      deviceConfig: ProjectDeviceConfig(
        slots: newSlots,
        requirePairing: state.deviceConfig.requirePairing,
      ),
    );
  }

  /// 更新槽位的设备ID和密钥
  void updateSlotDevice(String slotId, String deviceId, String secretKey) {
    final newSlots = state.deviceConfig.slots.map((s) {
      if (s.id == slotId) {
        s.defaultDeviceId = deviceId;
        s.defaultSecretKey = secretKey;
      }
      return s;
    }).toList();
    state = state.copyWith(
      deviceConfig: ProjectDeviceConfig(
        slots: newSlots,
        requirePairing: state.deviceConfig.requirePairing,
      ),
    );
  }

  /// 更新设备槽位
  void updateSlot(String slotId, {String? name, String? templateId}) {
    final newSlots = state.deviceConfig.slots.map((s) {
      if (s.id == slotId) {
        if (name != null) s.name = name;
        if (templateId != null) s.templateId = templateId;
      }
      return s;
    }).toList();
    state = state.copyWith(
      deviceConfig: ProjectDeviceConfig(
        slots: newSlots,
        requirePairing: state.deviceConfig.requirePairing,
      ),
    );
  }

  /// 添加应用设置项
  void addAppSetting(AppSetting setting) {
    final newSettings = [...state.appSettings.settings, setting];
    state = state.copyWith(
      appSettings: AppSettingsConfig(settings: newSettings),
    );
  }

  /// 更新应用设置项
  void updateAppSetting(String key, {String? label, String? defaultValue, bool? visible}) {
    final newSettings = state.appSettings.settings.map((s) {
      if (s.key == key) {
        if (label != null) s.label = label;
        if (defaultValue != null) s.defaultValue = defaultValue;
        if (visible != null) s.visible = visible;
      }
      return s;
    }).toList();
    state = state.copyWith(
      appSettings: AppSettingsConfig(settings: newSettings),
    );
  }

  /// 删除应用设置项
  void removeAppSetting(String key) {
    final newSettings = state.appSettings.settings.where((s) => s.key != key).toList();
    state = state.copyWith(
      appSettings: AppSettingsConfig(settings: newSettings),
    );
  }

  /// 切换设备管理功能
  void setDeviceManagementEnabled(bool enabled) {
    state = state.copyWith(
      deviceConfig: state.deviceConfig.copyWith(enableDeviceManagement: enabled),
    );
  }
}

/// Provider
final layoutCanvasProvider =
    StateNotifierProvider<LayoutCanvasNotifier, LayoutCanvasState>(
  (ref) => LayoutCanvasNotifier(),
);
