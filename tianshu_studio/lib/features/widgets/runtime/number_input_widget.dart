import 'package:flutter/material.dart';

/// 数值输入控件
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
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(color: Colors.black.withValues(alpha: 0.05), blurRadius: 10),
        ],
      ),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(label, style: TextStyle(fontSize: 14, color: Colors.grey[600])),
                Text('${value.toStringAsFixed(step < 1 ? 1 : 0)}',
                    style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
              ],
            ),
          ),
          IconButton(
            onPressed: onChanged == null ? null : () => onChanged!((value - step).clamp(min, max)),
            icon: const Icon(Icons.remove_circle_outline),
          ),
          IconButton(
            onPressed: onChanged == null ? null : () => onChanged!((value + step).clamp(min, max)),
            icon: const Icon(Icons.add_circle_outline),
          ),
        ],
      ),
    );
  }
}
