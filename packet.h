class Packet
{
public:
    Packet()
    {
            clear();
    }
    Packet(uint8_t cmd)
    {
            clear();
            m_cmd = cmd;
    }

    void setCmd(uint8_t cmd)
    {
        m_cmd = cmd;
    }

    void clear()
    {
            m_cmd = 0;
            m_len = 0;
            m_ritr = 0;
            m_recv = 0;
    }
    bool add(uint8_t ch)
    {     
        switch(m_recv)
        {
                case 0:
                    if(ch != 0xFF)
                        return false;
                    break;
                case 1:
                    if(ch != 0x00 && ch != 0x01)
                        return false;
                    break;
                case 2:
                    m_len = ch;
                    break;
                case 3:
                    m_cmd = ch;
                    break;
                default:
                {
                    if(m_recv-3 >= m_len)
                        return false;

                    m_data[m_recv - 4] = ch;
                    break;
                }
        }
        ++m_recv;
        return true;
    }
    bool isValid()
    {
        return (m_len && 3+m_len == m_recv);
    }
    void send()
    {
        uart.write(0xFF);
        uart.write(0x00);
        uart.write(m_len+1);
        uart.write(m_cmd);

        for(uint8_t i = 0; i < m_len; ++i)
            uart.write(m_data[i]);
    }

    uint8_t getCmd() const { return m_cmd; }
    
    void setWriteItr(int pos)
    {
        m_len = pos;
    }

    void write8(const uint8_t& v)
    {
        m_data[m_len++] = v;
    }
    void write16(const int16_t& v)
    {
        m_data[m_len++] = (v >> 8);
        m_data[m_len++] = (v & 0xFF);
    }
    void write32(const uint32_t& v)
    {
        m_data[m_len++] = (v >> 24);
        m_data[m_len++] = (v >> 16);
        m_data[m_len++] = (v >> 8);
        m_data[m_len++] = (v & 0xFF);
    }

    uint8_t read8()
    {
         return m_data[m_ritr++];
    }
    uint16_t read16()
    {
        uint16_t res = (m_data[m_ritr++] << 8);
        res |= m_data[m_ritr++];
        return res;
    }
    
    uint32_t read32()
    {
         uint32_t res = ((uint32_t)m_data[m_ritr++] << 24);
        res |= ((uint32_t)m_data[m_ritr++] << 16);
        res |= (m_data[m_ritr++] << 8);
        res |= m_data[m_ritr++];
        return res;
    }

private:
    uint8_t m_len;
    uint8_t m_cmd;
    uint8_t m_data[20];

    uint8_t m_recv;
    uint8_t m_ritr;
};