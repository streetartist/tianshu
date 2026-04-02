import 'package:flutter/material.dart';
import '../widgets/widgets.dart';
import '../services/iot_service.dart';

class MainPage extends StatefulWidget {
  const MainPage({super.key});

  @override
  State<MainPage> createState() => _MainPageState();
}

class _MainPageState extends State<MainPage> {
  final iot = IoTService.instance;

  @override
  void initState() {
    super.initState();
    iot.addListener(_onDataChanged);
  }

  @override
  void dispose() {
    iot.removeListener(_onDataChanged);
    super.dispose();
  }

  void _onDataChanged() => setState(() {});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('首页'),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          StatusLightWidget(
            label: '在线状态',
            status: iot.getBool('default', 'status'),
          ),
          ContainerWidget(
            title: '交互',
            layout: ContainerLayout.row,
            showBorder: false,
            children: [
              ButtonWidget(
                text: '喂食',
                onPressed: () => iot.send('default', 'pet_feed', null),
              ),
              ButtonWidget(
                text: '玩耍',
                onPressed: () => iot.send('default', 'pet_play', null),
              ),
              ButtonWidget(
                text: '休息',
                onPressed: () => iot.send('default', 'pet_sleep', null),
              ),
              ButtonWidget(
                text: '复活',
                onPressed: () => iot.send('default', 'pet_revive', null),
              ),
            ],
          ),
          ButtonWidget(
            text: '开始语音对话',
            onPressed: () => iot.send('default', 'start_chat', null),
          ),
          ContainerWidget(
            title: '数据',
            layout: ContainerLayout.row,
            showBorder: false,
            children: [
              NumberDisplayWidget(
                label: '温度',
                unit: '°C',
                value: iot.getValue('default', 'temperature'),
              ),
              NumberDisplayWidget(
                label: '湿度',
                unit: '%',
                value: iot.getValue('default', 'humidity'),
              ),
            ],
          ),
          ContainerWidget(
            title: null,
            layout: ContainerLayout.row,
            showBorder: false,
            children: [
              NumberDisplayWidget(
                label: '快乐值',
                unit: '',
                value: iot.getValue('default', 'pet_happiness'),
              ),
              NumberDisplayWidget(
                label: '饥饿值',
                unit: '',
                value: iot.getValue('default', 'pet_hunger'),
              ),
              NumberDisplayWidget(
                label: '疲倦值',
                unit: '°C',
                value: iot.getValue('default', 'pet_fatigue'),
              ),
            ],
          ),
          NumberDisplayWidget(
            label: '心情',
            unit: '',
            value: iot.getValue('default', 'pet_mood'),
          ),
          ContainerWidget(
            title: '数据图像',
            layout: ContainerLayout.column,
            showBorder: true,
            children: [
              LineChartWidget(
                title: '温度趋势',
                data: iot.getHistory('default', 'temperature'),
              ),
              LineChartWidget(
                title: '湿度趋势',
                data: iot.getHistory('default', 'humidity'),
              ),
              LineChartWidget(
                title: '饥饿值',
                data: iot.getHistory('default', 'pet_hunger'),
              ),
              LineChartWidget(
                title: '疲惫值',
                data: iot.getHistory('default', 'pet_fatigue'),
              ),
              LineChartWidget(
                title: '快乐值',
                data: iot.getHistory('default', 'pet_happiness'),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
