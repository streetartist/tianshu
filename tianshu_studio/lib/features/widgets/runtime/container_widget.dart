import 'package:flutter/material.dart';

/// 容器内部布局类型
enum ContainerLayout { column, row }

/// 容器控件 - 用于分组和布局
class ContainerWidget extends StatelessWidget {
  final String title;
  final bool showBorder;
  final bool showTitle;
  final ContainerLayout layout;
  final int gridColumns;
  final List<Widget> children;

  const ContainerWidget({
    super.key,
    required this.title,
    this.showBorder = true,
    this.showTitle = true,
    this.layout = ContainerLayout.column,
    this.gridColumns = 2,
    this.children = const [],
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        border: showBorder ? Border.all(color: Colors.grey.shade300) : null,
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.05),
            blurRadius: 10,
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          if (showTitle) ...[
            Text(
              title,
              style: TextStyle(
                fontSize: 14,
                fontWeight: FontWeight.bold,
                color: Colors.grey[700],
              ),
            ),
            const SizedBox(height: 8),
          ],
          _buildContent(),
        ],
      ),
    );
  }

  Widget _buildContent() {
    if (children.isEmpty) {
      return Container(
        height: 60,
        decoration: BoxDecoration(
          border: Border.all(color: Colors.grey.shade300, style: BorderStyle.solid),
          borderRadius: BorderRadius.circular(8),
          color: Colors.grey.shade50,
        ),
        child: const Center(
          child: Text('拖入控件', style: TextStyle(color: Colors.grey)),
        ),
      );
    }

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
