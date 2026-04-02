import 'package:flutter/material.dart';

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
    final percent = ((value - min) / (max - min)).clamp(0.0, 1.0);
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(color: Colors.black.withValues(alpha: 0.05), blurRadius: 10),
        ],
      ),
      child: Column(
        children: [
          Text(label, style: TextStyle(fontSize: 14, color: Colors.grey[600])),
          const SizedBox(height: 12),
          SizedBox(
            width: 100,
            height: 100,
            child: Stack(
              alignment: Alignment.center,
              children: [
                SizedBox(
                  width: 100,
                  height: 100,
                  child: CircularProgressIndicator(
                    value: percent,
                    strokeWidth: 10,
                    backgroundColor: Colors.grey[200],
                  ),
                ),
                Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text('${value.toStringAsFixed(1)}',
                        style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold)),
                    if (unit.isNotEmpty)
                      Text(unit, style: TextStyle(color: Colors.grey[600])),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
