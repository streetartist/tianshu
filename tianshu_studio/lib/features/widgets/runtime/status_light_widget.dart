import 'package:flutter/material.dart';

/// 状态指示灯控件
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
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 10),
        ],
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontSize: 16)),
          Container(
            width: 16,
            height: 16,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: status ? Colors.green : Colors.grey,
              boxShadow: status
                  ? [BoxShadow(color: Colors.green.withOpacity(0.5), blurRadius: 8)]
                  : null,
            ),
          ),
        ],
      ),
    );
  }
}
