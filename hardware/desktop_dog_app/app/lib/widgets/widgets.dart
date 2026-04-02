import 'package:flutter/material.dart';

/// 数值显示控件
class NumberDisplayWidget extends StatelessWidget {
  final String label;
  final double value;
  final String unit;

  const NumberDisplayWidget({
    super.key,
    required this.label,
    this.value = 0,
    this.unit = '',
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label, style: TextStyle(color: Colors.grey[600])),
            const SizedBox(height: 4),
            Text(
              '${value.toStringAsFixed(1)}$unit',
              style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
          ],
        ),
      ),
    );
  }
}

/// 开关控件
class SwitchWidget extends StatelessWidget {
  final String label;
  final bool value;
  final ValueChanged<bool>? onChanged;

  const SwitchWidget({
    super.key,
    required this.label,
    this.value = false,
    this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: const TextStyle(fontSize: 16)),
            Switch(value: value, onChanged: onChanged),
          ],
        ),
      ),
    );
  }
}

/// 文本显示控件
class TextDisplayWidget extends StatelessWidget {
  final String label;
  final String text;

  const TextDisplayWidget({
    super.key,
    required this.label,
    this.text = '',
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label, style: TextStyle(color: Colors.grey[600])),
            const SizedBox(height: 4),
            Text(text, style: const TextStyle(fontSize: 18)),
          ],
        ),
      ),
    );
  }
}

/// 状态灯控件
class StatusLightWidget extends StatelessWidget {
  final String label;
  final bool status;

  const StatusLightWidget({
    super.key,
    required this.label,
    this.status = false,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Container(
              width: 16,
              height: 16,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: status ? Colors.green : Colors.grey,
              ),
            ),
            const SizedBox(width: 12),
            Text(label, style: const TextStyle(fontSize: 16)),
          ],
        ),
      ),
    );
  }
}

/// 进度条控件
class ProgressBarWidget extends StatelessWidget {
  final String label;
  final double value;
  final double min;
  final double max;

  const ProgressBarWidget({
    super.key,
    required this.label,
    this.value = 0,
    this.min = 0,
    this.max = 100,
  });

  @override
  Widget build(BuildContext context) {
    final progress = (value - min) / (max - min);
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(label, style: TextStyle(color: Colors.grey[600])),
                Text('${value.toStringAsFixed(1)}'),
              ],
            ),
            const SizedBox(height: 8),
            LinearProgressIndicator(value: progress.clamp(0.0, 1.0)),
          ],
        ),
      ),
    );
  }
}

/// 仪表盘控件
class GaugeWidget extends StatelessWidget {
  final String label;
  final double value;
  final double min;
  final double max;
  final String unit;

  const GaugeWidget({
    super.key,
    required this.label,
    this.value = 0,
    this.min = 0,
    this.max = 100,
    this.unit = '',
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Text(label, style: TextStyle(color: Colors.grey[600])),
            const SizedBox(height: 8),
            Text(
              '${value.toStringAsFixed(1)}$unit',
              style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 4),
            Text('$min - $max', style: TextStyle(color: Colors.grey[400], fontSize: 12)),
          ],
        ),
      ),
    );
  }
}

/// 按钮控件
class ButtonWidget extends StatelessWidget {
  final String text;
  final VoidCallback? onPressed;

  const ButtonWidget({
    super.key,
    required this.text,
    this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: double.infinity,
      child: ElevatedButton(
        onPressed: onPressed,
        child: Text(text),
      ),
    );
  }
}

/// 滑块控件
class SliderWidget extends StatelessWidget {
  final String label;
  final double value;
  final double min;
  final double max;
  final ValueChanged<double>? onChanged;

  const SliderWidget({
    super.key,
    required this.label,
    this.value = 0,
    this.min = 0,
    this.max = 100,
    this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(label, style: TextStyle(color: Colors.grey[600])),
                Text(value.toStringAsFixed(1)),
              ],
            ),
            Slider(
              value: value.clamp(min, max),
              min: min,
              max: max,
              onChanged: onChanged,
            ),
          ],
        ),
      ),
    );
  }
}

/// 下拉选择控件
class DropdownWidget extends StatelessWidget {
  final String label;
  final String value;
  final List<String> options;
  final ValueChanged<String?>? onChanged;

  const DropdownWidget({
    super.key,
    required this.label,
    this.value = '',
    this.options = const [],
    this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: const TextStyle(fontSize: 16)),
            DropdownButton<String>(
              value: options.contains(value) ? value : null,
              items: options.map((o) => DropdownMenuItem(value: o, child: Text(o))).toList(),
              onChanged: onChanged,
            ),
          ],
        ),
      ),
    );
  }
}

/// 数字输入控件
class NumberInputWidget extends StatelessWidget {
  final String label;
  final double value;
  final double min;
  final double max;
  final double step;
  final ValueChanged<double>? onChanged;

  const NumberInputWidget({
    super.key,
    required this.label,
    this.value = 0,
    this.min = 0,
    this.max = 100,
    this.step = 1,
    this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: const TextStyle(fontSize: 16)),
            Row(
              children: [
                IconButton(
                  icon: const Icon(Icons.remove),
                  onPressed: value > min ? () => onChanged?.call(value - step) : null,
                ),
                Text(value.toStringAsFixed(1)),
                IconButton(
                  icon: const Icon(Icons.add),
                  onPressed: value < max ? () => onChanged?.call(value + step) : null,
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

/// 折线图控件
class LineChartWidget extends StatelessWidget {
  final String title;
  final List<double> data;

  const LineChartWidget({
    super.key,
    required this.title,
    this.data = const [],
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: const TextStyle(fontWeight: FontWeight.bold)),
            const SizedBox(height: 8),
            SizedBox(
              height: 160,
              child: CustomPaint(
                size: const Size(double.infinity, 160),
                painter: _LineChartPainter(data),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _LineChartPainter extends CustomPainter {
  final List<double> data;
  _LineChartPainter(this.data);

  @override
  void paint(Canvas canvas, Size size) {
    if (data.isEmpty) return;

    const leftMargin = 44.0;
    const bottomMargin = 24.0;
    const topPadding = 8.0;
    final chartWidth = size.width - leftMargin;
    final chartHeight = size.height - bottomMargin - topPadding;

    final axisPaint = Paint()
      ..color = Colors.grey.shade400
      ..strokeWidth = 1;
    final gridPaint = Paint()
      ..color = Colors.grey.shade200
      ..strokeWidth = 0.5;
    final linePaint = Paint()
      ..color = Colors.blue
      ..strokeWidth = 2
      ..style = PaintingStyle.stroke;
    final labelStyle = TextStyle(color: Colors.grey.shade600, fontSize: 10);

    final maxVal = data.reduce((a, b) => a > b ? a : b);
    final minVal = data.reduce((a, b) => a < b ? a : b);
    final range = maxVal - minVal;

    // --- Y 轴刻度 ---
    const yTickCount = 4;
    for (var i = 0; i <= yTickCount; i++) {
      final ratio = i / yTickCount;
      final y = topPadding + chartHeight * (1 - ratio);
      // 网格线
      canvas.drawLine(Offset(leftMargin, y), Offset(size.width, y), gridPaint);
      // 刻度值
      final val = range == 0
          ? (minVal - 2 + ratio * 4)
          : minVal + range * ratio;
      final tp = TextPainter(
        text: TextSpan(text: val.toStringAsFixed(1), style: labelStyle),
        textDirection: TextDirection.ltr,
      )..layout();
      tp.paint(canvas, Offset(leftMargin - tp.width - 6, y - tp.height / 2));
    }

    // --- X 轴刻度 ---
    final xTickCount = data.length <= 1 ? 1 : (data.length > 10 ? 5 : data.length - 1);
    for (var i = 0; i <= xTickCount; i++) {
      final dataIdx = data.length <= 1
          ? 0
          : (i * (data.length - 1) / xTickCount).round();
      final x = leftMargin + (data.length <= 1 ? chartWidth / 2 : dataIdx * chartWidth / (data.length - 1));
      // 刻度线
      canvas.drawLine(Offset(x, topPadding + chartHeight), Offset(x, topPadding + chartHeight + 4), axisPaint);
      // 序号标签
      final tp = TextPainter(
        text: TextSpan(text: '${dataIdx + 1}', style: labelStyle),
        textDirection: TextDirection.ltr,
      )..layout();
      tp.paint(canvas, Offset(x - tp.width / 2, topPadding + chartHeight + 6));
    }

    // --- 坐标轴 ---
    canvas.drawLine(Offset(leftMargin, topPadding), Offset(leftMargin, topPadding + chartHeight), axisPaint);
    canvas.drawLine(Offset(leftMargin, topPadding + chartHeight), Offset(size.width, topPadding + chartHeight), axisPaint);

    // --- 数据线 ---
    if (data.length == 1) {
      final cx = leftMargin + chartWidth / 2;
      final cy = topPadding + chartHeight / 2;
      canvas.drawCircle(Offset(cx, cy), 3, linePaint..style = PaintingStyle.fill);
      return;
    }

    final path = Path();
    for (var i = 0; i < data.length; i++) {
      final x = leftMargin + i * chartWidth / (data.length - 1);
      final normalized = range == 0 ? 0.5 : (data[i] - minVal) / range;
      final y = topPadding + chartHeight * (1 - normalized);
      if (i == 0) {
        path.moveTo(x, y);
      } else {
        path.lineTo(x, y);
      }
    }
    canvas.drawPath(path, linePaint..style = PaintingStyle.stroke);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}

/// 柱状图控件
class BarChartWidget extends StatelessWidget {
  final String title;
  final Map<String, double> data;

  const BarChartWidget({
    super.key,
    required this.title,
    this.data = const {},
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: const TextStyle(fontWeight: FontWeight.bold)),
            const SizedBox(height: 8),
            SizedBox(
              height: 120,
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: data.entries.map((e) {
                  final maxVal = data.values.isEmpty ? 1 : data.values.reduce((a, b) => a > b ? a : b);
                  final h = maxVal == 0 ? 0.0 : (e.value / maxVal) * 100;
                  return Expanded(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.end,
                      children: [
                        Container(height: h, color: Colors.blue, margin: const EdgeInsets.symmetric(horizontal: 2)),
                        const SizedBox(height: 4),
                        Text(e.key, style: const TextStyle(fontSize: 10)),
                      ],
                    ),
                  );
                }).toList(),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// 容器布局类型
enum ContainerLayout { column, row }

/// 容器控件
class ContainerWidget extends StatelessWidget {
  final String? title;
  final ContainerLayout layout;
  final bool showBorder;
  final List<Widget> children;

  const ContainerWidget({
    super.key,
    this.title,
    this.layout = ContainerLayout.column,
    this.showBorder = true,
    this.children = const [],
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisSize: MainAxisSize.min,
          children: [
            if (title != null) ...[
              Text(title!, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
              const SizedBox(height: 8),
            ],
            _buildLayout(),
          ],
        ),
      ),
    );
  }

  Widget _buildLayout() {
    switch (layout) {
      case ContainerLayout.row:
        return Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: children.asMap().entries.map((e) {
            final isLast = e.key == children.length - 1;
            return Expanded(
              child: Padding(
                padding: EdgeInsets.only(right: isLast ? 0 : 8),
                child: e.value,
              ),
            );
          }).toList(),
        );
      case ContainerLayout.column:
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          mainAxisSize: MainAxisSize.min,
          children: children.asMap().entries.map((e) {
            final isLast = e.key == children.length - 1;
            return Padding(
              padding: EdgeInsets.only(bottom: isLast ? 0 : 8),
              child: e.value,
            );
          }).toList(),
        );
    }
  }
}
