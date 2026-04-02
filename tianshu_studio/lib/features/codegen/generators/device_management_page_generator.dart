import '../../../data/models/device_slot.dart';
import '../models/generated_project.dart';

/// 设备管理页面生成器
class DeviceManagementPageGenerator {
  /// 生成设备管理页面
  static GeneratedFile generate(List<DeviceSlot> slots) {
    final defaultDevices = _generateDefaultDevices(slots);

    return GeneratedFile(
      path: 'lib/pages/device_management_page.dart',
      content: _buildPageContent(defaultDevices),
    );
  }

  static String _generateDefaultDevices(List<DeviceSlot> slots) {
    final items = <String>[];
    for (final slot in slots) {
      if (slot.defaultDeviceId.isNotEmpty) {
        items.add('''      DeviceInfo(
        id: '${slot.defaultDeviceId}',
        name: '${slot.name}',
        secretKey: '${slot.defaultSecretKey}',
      ),''');
      }
    }
    return items.join('\n');
  }

  static String _buildPageContent(String defaultDevices) {
    return r'''import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/iot_service.dart';

/// 设备信息
class DeviceInfo {
  String id;
  String name;
  String secretKey;
  bool isDefault;

  DeviceInfo({
    required this.id,
    required this.name,
    this.secretKey = '',
    this.isDefault = false,
  });

  Map<String, dynamic> toJson() => {
    'id': id,
    'name': name,
    'secretKey': secretKey,
    'isDefault': isDefault,
  };

  factory DeviceInfo.fromJson(Map<String, dynamic> json) => DeviceInfo(
    id: json['id'] ?? '',
    name: json['name'] ?? '',
    secretKey: json['secretKey'] ?? '',
    isDefault: json['isDefault'] ?? false,
  );
}

/// 设备管理页面
class DeviceManagementPage extends StatefulWidget {
  const DeviceManagementPage({super.key});

  @override
  State<DeviceManagementPage> createState() => _DeviceManagementPageState();
}

class _DeviceManagementPageState extends State<DeviceManagementPage> {
  final _devices = <DeviceInfo>[];
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _loadDevices();
  }

  Future<void> _loadDevices() async {
    final prefs = await SharedPreferences.getInstance();
    final json = prefs.getString('devices_list');

    if (json != null && json.isNotEmpty) {
      final list = jsonDecode(json) as List;
      _devices.addAll(list.map((e) => DeviceInfo.fromJson(e)));
    } else {
      // 加载默认设备
      _devices.addAll(_getDefaultDevices());
      await _saveDevices();
    }

    // 绑定默认设备到IoT服务
    final defaultDevice = _devices.where((d) => d.isDefault).firstOrNull ?? _devices.firstOrNull;
    if (defaultDevice != null) {
      IoTService.instance.bindDevice('default', defaultDevice.id, defaultDevice.secretKey);
    }

    setState(() => _loading = false);
  }

  List<DeviceInfo> _getDefaultDevices() {
    return [
''' + defaultDevices + r'''
    ];
  }

  Future<void> _saveDevices() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('devices_list', jsonEncode(_devices.map((d) => d.toJson()).toList()));
  }

  void _setDefaultDevice(DeviceInfo device) {
    setState(() {
      for (final d in _devices) {
        d.isDefault = d.id == device.id;
      }
    });
    _saveDevices();
    IoTService.instance.bindDevice('default', device.id, device.secretKey);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('已切换到设备: ${device.name}')),
    );
  }

  void _addDevice() {
    _showDeviceDialog(null);
  }

  void _editDevice(DeviceInfo device) {
    _showDeviceDialog(device);
  }

  void _deleteDevice(DeviceInfo device) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('删除设备'),
        content: Text('确定要删除设备 "${device.name}" 吗？'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () {
              Navigator.pop(ctx);
              setState(() => _devices.remove(device));
              _saveDevices();
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('设备已删除')),
              );
            },
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('删除'),
          ),
        ],
      ),
    );
  }

  void _showDeviceDialog(DeviceInfo? device) {
    final isEdit = device != null;
    final nameController = TextEditingController(text: device?.name ?? '');
    final idController = TextEditingController(text: device?.id ?? '');
    final keyController = TextEditingController(text: device?.secretKey ?? '');

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(isEdit ? '编辑设备' : '添加设备'),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: nameController,
                decoration: const InputDecoration(
                  labelText: '设备名称',
                  hintText: '给设备起个名字',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: idController,
                decoration: const InputDecoration(
                  labelText: '设备ID',
                  hintText: '输入设备ID',
                  border: OutlineInputBorder(),
                ),
                enabled: !isEdit,
              ),
              const SizedBox(height: 12),
              TextField(
                controller: keyController,
                decoration: const InputDecoration(
                  labelText: '设备密钥',
                  hintText: '输入设备密钥',
                  border: OutlineInputBorder(),
                ),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('取消'),
          ),
          ElevatedButton(
            onPressed: () {
              final name = nameController.text.trim();
              final id = idController.text.trim();
              final key = keyController.text.trim();

              if (name.isEmpty || id.isEmpty) {
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('请填写设备名称和ID')),
                );
                return;
              }

              Navigator.pop(ctx);

              if (isEdit) {
                setState(() {
                  device.name = name;
                  device.secretKey = key;
                });
              } else {
                setState(() {
                  _devices.add(DeviceInfo(
                    id: id,
                    name: name,
                    secretKey: key,
                    isDefault: _devices.isEmpty,
                  ));
                });
              }
              _saveDevices();
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text(isEdit ? '设备已更新' : '设备已添加')),
              );
            },
            child: const Text('保存'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('设备管理'),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _addDevice,
        child: const Icon(Icons.add),
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : _devices.isEmpty
              ? _buildEmptyState()
              : _buildDeviceList(),
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(Icons.devices_other, size: 64, color: Colors.grey[400]),
          const SizedBox(height: 16),
          Text('暂无设备', style: TextStyle(color: Colors.grey[600], fontSize: 16)),
          const SizedBox(height: 8),
          ElevatedButton.icon(
            onPressed: _addDevice,
            icon: const Icon(Icons.add),
            label: const Text('添加设备'),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceList() {
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: _devices.length,
      itemBuilder: (context, index) {
        final device = _devices[index];
        return _buildDeviceCard(device);
      },
    );
  }

  Widget _buildDeviceCard(DeviceInfo device) {
    final isOnline = IoTService.instance.isOnline('default');

    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: InkWell(
        onTap: () => _setDefaultDevice(device),
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              // 设备图标和状态
              Stack(
                children: [
                  Container(
                    width: 48,
                    height: 48,
                    decoration: BoxDecoration(
                      color: device.isDefault ? Colors.blue.shade50 : Colors.grey.shade100,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Icon(
                      Icons.developer_board,
                      color: device.isDefault ? Colors.blue : Colors.grey,
                    ),
                  ),
                  if (device.isDefault)
                    Positioned(
                      right: 0,
                      bottom: 0,
                      child: Container(
                        width: 14,
                        height: 14,
                        decoration: BoxDecoration(
                          color: isOnline ? Colors.green : Colors.grey,
                          shape: BoxShape.circle,
                          border: Border.all(color: Colors.white, width: 2),
                        ),
                      ),
                    ),
                ],
              ),
              const SizedBox(width: 16),
              // 设备信息
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Text(
                          device.name,
                          style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                        if (device.isDefault) ...[
                          const SizedBox(width: 8),
                          Container(
                            padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                            decoration: BoxDecoration(
                              color: Colors.blue,
                              borderRadius: BorderRadius.circular(4),
                            ),
                            child: const Text(
                              '当前',
                              style: TextStyle(color: Colors.white, fontSize: 10),
                            ),
                          ),
                        ],
                      ],
                    ),
                    const SizedBox(height: 4),
                    Text(
                      'ID: ${device.id.length > 20 ? '${device.id.substring(0, 20)}...' : device.id}',
                      style: TextStyle(color: Colors.grey[600], fontSize: 12),
                    ),
                  ],
                ),
              ),
              // 操作按钮
              PopupMenuButton<String>(
                onSelected: (value) {
                  switch (value) {
                    case 'edit':
                      _editDevice(device);
                      break;
                    case 'delete':
                      _deleteDevice(device);
                      break;
                    case 'use':
                      _setDefaultDevice(device);
                      break;
                  }
                },
                itemBuilder: (context) => [
                  if (!device.isDefault)
                    const PopupMenuItem(value: 'use', child: Text('使用此设备')),
                  const PopupMenuItem(value: 'edit', child: Text('编辑')),
                  const PopupMenuItem(value: 'delete', child: Text('删除')),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
''';
  }
}
