import 'package:flutter/material.dart';

/// 设置页面
class SettingsPage extends StatelessWidget {
  const SettingsPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('设置')),
      body: ListView(
        children: [
          _buildSection('服务器配置', [
            _buildTextField('服务器地址', 'http://localhost:5000'),
            _buildTextField('超时时间(ms)', '30000'),
          ]),
          _buildSection('界面设置', [
            _buildSwitchTile('暗色主题', false),
            _buildDropdown('语言', '中文'),
          ]),
        ],
      ),
    );
  }

  Widget _buildSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: Text(title,
              style: const TextStyle(
                  fontSize: 16, fontWeight: FontWeight.bold)),
        ),
        ...children,
        const Divider(),
      ],
    );
  }

  Widget _buildTextField(String label, String value) {
    return ListTile(
      title: Text(label),
      subtitle: Text(value),
      trailing: const Icon(Icons.edit),
    );
  }

  Widget _buildSwitchTile(String title, bool value) {
    return SwitchListTile(
      title: Text(title),
      value: value,
      onChanged: (v) {},
    );
  }

  Widget _buildDropdown(String title, String value) {
    return ListTile(
      title: Text(title),
      trailing: Text(value),
    );
  }
}
