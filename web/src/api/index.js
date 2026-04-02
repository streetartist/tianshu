import request from './request'

export default {
  auth: {
    login: (username, password) => request.post('/auth/login', { username, password }),
    register: (data) => request.post('/auth/register', data),
    me: () => request.get('/auth/me')
  },
  devices: {
    list: () => request.get('/devices'),
    create: (data) => request.post('/devices', data),
    get: (id) => request.get(`/devices/${id}`),
    update: (id, data) => request.put(`/devices/${id}`, data),
    delete: (id) => request.delete(`/devices/${id}`),
    getConfig: (id) => request.get(`/devices/${id}/config`),
    updateConfig: (id, data) => request.put(`/devices/${id}/config`, data),
    getSecret: (id) => request.get(`/devices/${id}/secret`),
    resetSecret: (id) => request.post(`/devices/${id}/secret`),
    checkStatus: () => request.post('/devices/check-status')
  },
  data: {
    report: (data) => request.post('/data/report', data),
    get: (deviceId, params) => request.get(`/data/${deviceId}`, { params })
  },
  gateway: {
    list: () => request.get('/gateway/configs'),
    create: (data) => request.post('/gateway/configs', data),
    get: (id) => request.get(`/gateway/configs/${id}`),
    update: (id, data) => request.put(`/gateway/configs/${id}`, data),
    delete: (id) => request.delete(`/gateway/configs/${id}`),
    proxy: (id, data) => request.post(`/gateway/proxy/${id}`, data)
  },
  agents: {
    list: () => request.get('/agents'),
    create: (data) => request.post('/agents', data),
    get: (id) => request.get(`/agents/${id}`),
    update: (id, data) => request.put(`/agents/${id}`, data),
    delete: (id) => request.delete(`/agents/${id}`),
    bind: (agentId, deviceId) => request.post(`/agents/${agentId}/bind`, { device_id: deviceId }),
    chat: (agentId, message, conversationId) => request.post(`/agents/${agentId}/chat`, { message, conversation_id: conversationId }),
    conversations: (agentId) => request.get(`/agents/${agentId}/conversations`),
    getConversation: (agentId, convId) => request.get(`/agents/${agentId}/conversations/${convId}`),
    deleteConversation: (agentId, convId) => request.delete(`/agents/${agentId}/conversations/${convId}`),
    tools: () => request.get('/agents/tools')
  },
  templates: {
    list: () => request.get('/templates'),
    get: (id) => request.get(`/templates/${id}`),
    create: (data) => request.post('/templates', data),
    update: (id, data) => request.put(`/templates/${id}`, data),
    delete: (id) => request.delete(`/templates/${id}`)
  },
  logs: {
    list: (params) => request.get('/logs', { params })
  }
}
