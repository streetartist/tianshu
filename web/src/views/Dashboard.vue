<template>
  <div class="dashboard">
    <h2>仪表盘</h2>

    <!-- 统计卡片 -->
    <el-row :gutter="20" class="stats">
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-item">
            <el-icon class="icon" style="color: #409eff"><Cpu /></el-icon>
            <div class="info">
              <div class="value">{{ stats.devices }}</div>
              <div class="label">设备总数</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-item">
            <el-icon class="icon" style="color: #67c23a"><CircleCheck /></el-icon>
            <div class="info">
              <div class="value">{{ stats.online }}</div>
              <div class="label">在线设备</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-item">
            <el-icon class="icon" style="color: #e6a23c"><Connection /></el-icon>
            <div class="info">
              <div class="value">{{ stats.apis }}</div>
              <div class="label">API配置</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover">
          <div class="stat-item">
            <el-icon class="icon" style="color: #f56c6c"><MagicStick /></el-icon>
            <div class="info">
              <div class="value">{{ stats.agents }}</div>
              <div class="label">AI Agent</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 快速操作 -->
    <el-card class="quick-actions">
      <template #header>快速操作</template>
      <el-space>
        <el-button type="primary" @click="$router.push('/devices')">
          <el-icon><Plus /></el-icon> 添加设备
        </el-button>
        <el-button @click="$router.push('/gateway')">
          <el-icon><Connection /></el-icon> 配置API
        </el-button>
        <el-button @click="$router.push('/agents')">
          <el-icon><MagicStick /></el-icon> 创建Agent
        </el-button>
      </el-space>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '@/api'

const stats = ref({
  devices: 0,
  online: 0,
  apis: 0,
  agents: 0
})

onMounted(async () => {
  try {
    const [devices, configs, agents] = await Promise.all([
      api.devices.list(),
      api.gateway.list(),
      api.agents.list()
    ])
    stats.value.devices = devices.data?.length || 0
    stats.value.online = devices.data?.filter(d => d.status === 'online').length || 0
    stats.value.apis = configs.data?.length || 0
    stats.value.agents = agents.data?.length || 0
  } catch (e) {
    console.error(e)
  }
})
</script>

<style scoped>
.dashboard h2 {
  margin: 0 0 20px;
}
.stats {
  margin-bottom: 20px;
}
.stat-item {
  display: flex;
  align-items: center;
  gap: 16px;
}
.stat-item .icon {
  font-size: 48px;
}
.stat-item .value {
  font-size: 28px;
  font-weight: bold;
}
.stat-item .label {
  color: #909399;
}
.quick-actions {
  margin-top: 20px;
}
</style>
