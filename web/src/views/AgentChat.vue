<template>
  <div class="agent-chat">
    <el-page-header @back="$router.back()">
      <template #content>
        <el-icon><component :is="agent.icon || 'Robot'" /></el-icon>
        <span style="margin-left: 8px">{{ agent.name }}</span>
      </template>
    </el-page-header>

    <div class="chat-layout">
      <!-- 对话历史侧边栏 -->
      <div class="sidebar">
        <div class="sidebar-header">
          <span>对话历史</span>
          <el-button size="small" @click="newConversation">新对话</el-button>
        </div>
        <div class="conversation-list">
          <div
            v-for="conv in conversations"
            :key="conv.id"
            :class="['conv-item', { active: currentConvId === conv.id }]"
            @click="loadConversation(conv.id)"
          >
            <div class="conv-title">{{ conv.title || '新对话' }}</div>
            <div class="conv-meta">
              <span>{{ conv.message_count }}条消息</span>
              <el-button link size="small" @click.stop="deleteConv(conv.id)">
                <el-icon><Delete /></el-icon>
              </el-button>
            </div>
          </div>
          <el-empty v-if="!conversations.length" description="暂无对话" :image-size="60" />
        </div>
      </div>

      <!-- 聊天区域 -->
      <div class="chat-container">
        <div class="messages" ref="messagesRef">
          <div
            v-for="(msg, i) in messages"
            :key="i"
            :class="['message', msg.role]"
          >
            <div class="avatar">
              <el-icon v-if="msg.role === 'assistant'"><component :is="agent.icon || 'Robot'" /></el-icon>
              <el-icon v-else><User /></el-icon>
            </div>
            <div class="content">
              <!-- 多轮过程显示 -->
              <div v-if="msg.rounds && msg.rounds.length > 1" class="rounds-container">
                <el-collapse class="rounds-collapse">
                  <el-collapse-item>
                    <template #title>
                      <el-icon><List /></el-icon>
                      <span style="margin-left: 4px">经过 {{ msg.rounds.length }} 轮思考</span>
                    </template>
                    <div v-for="(round, ri) in msg.rounds.slice(0, -1)" :key="ri" class="round-item">
                      <div class="round-header">第 {{ round.round }} 轮</div>
                      <div v-if="round.content" class="round-content">{{ round.content }}</div>
                      <div v-for="(t, ti) in round.tools" :key="ti" class="round-tool">
                        <span class="tool-badge">{{ t.tool }}</span>
                        <span v-if="t.result" class="tool-result-mini">{{ JSON.stringify(t.result).slice(0, 100) }}</span>
                      </div>
                    </div>
                  </el-collapse-item>
                </el-collapse>
              </div>
              <!-- 最终回复 -->
              <div class="text" v-if="msg.content">{{ msg.content }}</div>
            </div>
          </div>
        </div>

        <div class="input-area">
          <el-input
            v-model="input"
            placeholder="输入消息..."
            @keyup.enter="sendMessage"
            :disabled="loading"
          />
          <el-button type="primary" @click="sendMessage" :loading="loading">
            发送
          </el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'

const route = useRoute()
const agentId = route.params.id

const agent = ref({})
const conversations = ref([])
const messages = ref([])
const input = ref('')
const loading = ref(false)
const messagesRef = ref()
const currentConvId = ref(null)

const loadAgent = async () => {
  const res = await api.agents.get(agentId)
  if (res.code === 0) {
    agent.value = res.data
  }
}

const loadConversations = async () => {
  const res = await api.agents.conversations(agentId)
  conversations.value = res.data || []
}

const loadConversation = async (convId) => {
  currentConvId.value = convId
  const res = await api.agents.getConversation(agentId, convId)
  if (res.code === 0) {
    messages.value = res.data.messages || []
    await nextTick()
    scrollToBottom()
  }
}

const newConversation = () => {
  currentConvId.value = null
  messages.value = []
}

const deleteConv = async (convId) => {
  await ElMessageBox.confirm('确定删除该对话？', '提示', { type: 'warning' })
  const res = await api.agents.deleteConversation(agentId, convId)
  if (res.code === 0) {
    ElMessage.success('删除成功')
    if (currentConvId.value === convId) {
      newConversation()
    }
    loadConversations()
  }
}

const scrollToBottom = () => {
  messagesRef.value?.scrollTo(0, messagesRef.value.scrollHeight)
}

const statusText = ref('')

const sendMessage = async () => {
  if (!input.value.trim() || loading.value) return

  const msg = input.value
  input.value = ''
  messages.value.push({ role: 'user', content: msg })

  // 添加assistant消息
  const msgIndex = messages.value.length
  messages.value.push({
    role: 'assistant',
    content: '',
    tool_results: [],
    rounds: []
  })

  loading.value = true
  await nextTick()
  scrollToBottom()

  const collectedTools = []
  let currentRound = 0

  try {
    const token = localStorage.getItem('token')
    // 直接连接后端，绕过Vite代理以支持真正的流式输出
    const streamUrl = `http://localhost:5000/api/v1/agents/${agentId}/chat/stream`
    const response = await fetch(streamUrl, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${token}`
      },
      body: JSON.stringify({ message: msg, conversation_id: currentConvId.value })
    })

    const reader = response.body.getReader()
    const decoder = new TextDecoder()
    let buffer = ''

    while (true) {
      const { done, value } = await reader.read()
      if (done) break

      buffer += decoder.decode(value, { stream: true })
      const lines = buffer.split('\n')
      buffer = lines.pop() || ''

      for (const line of lines) {
        if (!line.startsWith('data: ')) continue
        try {
          const event = JSON.parse(line.slice(6))
          const currentMsg = messages.value[msgIndex]

          if (event.type === 'round') {
            currentRound = event.round
            currentMsg.rounds.push({ round: currentRound, content: '', tools: [] })
            messages.value = [...messages.value]
          } else if (event.type === 'content') {
            // 打字机效果：逐字添加
            currentMsg.content += event.text
            if (currentMsg.rounds.length > 0) {
              currentMsg.rounds[currentMsg.rounds.length - 1].content += event.text
            }
            messages.value = [...messages.value]
          } else if (event.type === 'tool_call') {
            collectedTools.push({ tool: event.tool, args: event.args, result: null })
            currentMsg.tool_results = [...collectedTools]
            if (currentMsg.rounds.length > 0) {
              currentMsg.rounds[currentMsg.rounds.length - 1].tools.push({
                tool: event.tool, args: event.args, result: null
              })
            }
            messages.value = [...messages.value]
          } else if (event.type === 'tool_result') {
            const tc = collectedTools.find(t => t.tool === event.tool && !t.result)
            if (tc) tc.result = event.result
            currentMsg.tool_results = [...collectedTools]
            if (currentMsg.rounds.length > 0) {
              const roundTools = currentMsg.rounds[currentMsg.rounds.length - 1].tools
              const rtc = roundTools.find(t => t.tool === event.tool && !t.result)
              if (rtc) rtc.result = event.result
            }
            // 清空content准备下一轮
            currentMsg.content = ''
            messages.value = [...messages.value]
          } else if (event.type === 'done') {
            currentMsg.content = event.content
            currentMsg.tool_results = collectedTools
            if (event.conversation_id) {
              currentConvId.value = event.conversation_id
            }
            messages.value = [...messages.value]
          } else if (event.type === 'error') {
            currentMsg.content = `错误: ${event.message}`
            messages.value = [...messages.value]
          }

          await nextTick()
          scrollToBottom()
        } catch (e) { }
      }
    }

    loadConversations()
  } catch (e) {
    messages.value[msgIndex].content = `请求异常: ${e.message || e}`
    messages.value = [...messages.value]
  } finally {
    loading.value = false
    await nextTick()
    scrollToBottom()
  }
}

onMounted(() => {
  loadAgent()
  loadConversations()
})
</script>

<style scoped>
.chat-layout {
  display: flex;
  margin-top: 20px;
  height: calc(100vh - 180px);
  gap: 16px;
}
.sidebar {
  width: 260px;
  background: #f5f7fa;
  border-radius: 8px;
  display: flex;
  flex-direction: column;
}
.sidebar-header {
  padding: 12px 16px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid #e4e7ed;
  font-weight: 500;
}
.conversation-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}
.conv-item {
  padding: 10px 12px;
  border-radius: 6px;
  cursor: pointer;
  margin-bottom: 4px;
}
.conv-item:hover { background: #e9ecef; }
.conv-item.active { background: #409eff20; }
.conv-title {
  font-size: 14px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.conv-meta {
  font-size: 12px;
  color: #909399;
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 4px;
}
.chat-container {
  flex: 1;
  display: flex;
  flex-direction: column;
}
.messages {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  background: #fafafa;
  border-radius: 8px;
}
.message {
  margin-bottom: 16px;
  display: flex;
  gap: 12px;
}
.message.user { flex-direction: row-reverse; }
.message .avatar {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: #e4e7ed;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}
.message.assistant .avatar { background: #409eff; color: #fff; }
.message .content {
  max-width: 70%;
  padding: 12px 16px;
  border-radius: 8px;
  line-height: 1.5;
  white-space: pre-wrap;
}
.message.user .content { background: #409eff; color: #fff; }
.message.assistant .content { background: #fff; box-shadow: 0 1px 2px rgba(0,0,0,0.1); }
.typing { color: #909399; }
.rounds-container {
  margin-bottom: 12px;
}
.rounds-collapse {
  --el-collapse-header-bg-color: #f0f9ff;
  --el-collapse-content-bg-color: #f8fafc;
}
.rounds-collapse :deep(.el-collapse-item__header) {
  font-size: 12px;
  color: #409eff;
  height: 32px;
}
.round-item {
  padding: 8px;
  margin-bottom: 8px;
  background: #fff;
  border-radius: 4px;
  border-left: 3px solid #409eff;
}
.round-header {
  font-size: 12px;
  font-weight: 600;
  color: #409eff;
  margin-bottom: 4px;
}
.round-content {
  font-size: 13px;
  color: #606266;
  margin-bottom: 4px;
}
.round-tool {
  font-size: 12px;
  margin-top: 4px;
}
.tool-badge {
  background: #e6f7ff;
  color: #1890ff;
  padding: 2px 6px;
  border-radius: 3px;
  margin-right: 8px;
}
.tool-result-mini {
  color: #909399;
  word-break: break-all;
}
.status-text {
  color: #409eff;
  font-size: 13px;
  margin-bottom: 8px;
}
.thinking-text {
  color: #909399;
  font-style: italic;
  margin-bottom: 8px;
  padding: 8px;
  background: #f5f7fa;
  border-radius: 4px;
  font-size: 13px;
}
.tool-collapse {
  margin-bottom: 8px;
  --el-collapse-header-bg-color: #f0f9ff;
  --el-collapse-content-bg-color: #f8fafc;
}
.tool-collapse :deep(.el-collapse-item__header) {
  font-size: 12px;
  color: #409eff;
  height: 32px;
}
.tool-item {
  padding: 8px;
  margin-bottom: 6px;
  background: #fff;
  border-radius: 4px;
  font-size: 12px;
}
.tool-name {
  font-weight: 600;
  color: #303133;
}
.tool-args, .tool-result {
  color: #606266;
  margin-top: 4px;
  word-break: break-all;
}
.input-area {
  display: flex;
  gap: 10px;
  margin-top: 16px;
}
.input-area .el-input { flex: 1; }
</style>
