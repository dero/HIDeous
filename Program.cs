using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Runtime.Versioning;
using System.Collections.Generic;
using System.Drawing;

[SupportedOSPlatform("windows")]
public class MainForm : Form
{
    private TableLayoutPanel table;
    private Dictionary<IntPtr, TableRow> deviceRows = new();
    private const int WM_INPUT = 0x00FF;
    private const uint RIM_TYPEKEYBOARD = 1;

    public class TableRow
    {
        public CheckBox Override { get; set; }
        public TextBox CustomName { get; set; }
        public Label DeviceId { get; set; }
        public Label Keycode { get; set; }

        // For handling the flash effect
        private Color originalColor;
        private Timer flashTimer;

        public TableRow()
        {
            Override = new CheckBox() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = ContentAlignment.MiddleCenter };
            CustomName = new TextBox() { Dock = DockStyle.Fill, TextAlign = HorizontalAlignment.Center };
            DeviceId = new Label() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = ContentAlignment.MiddleCenter };
            Keycode = new Label() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = ContentAlignment.MiddleCenter };

            // Setup flash timer
            flashTimer = new Timer();
            flashTimer.Interval = 100; // 100ms flash duration
            flashTimer.Tick += (s, e) =>
            {
                Keycode.BackColor = originalColor;
                flashTimer.Stop();
            };

            originalColor = Keycode.BackColor;
        }

        public void Flash()
        {
            Keycode.BackColor = Color.LightBlue;
            flashTimer.Stop(); // Reset any ongoing flash
            flashTimer.Start();
        }
    }

    public MainForm()
    {
        Text = "Keyboard Mapper";
        Size = new Size(800, 300);

        // Create main table layout
        table = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 4,
            RowCount = 1, // Header row
            CellBorderStyle = TableLayoutPanelCellBorderStyle.Single
        };

        // Configure columns
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 70)); // Override
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 40));  // Custom Name
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));  // Device ID
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));  // Keycode

        // Add headers
        AddHeader("Override", 0);
        AddHeader("Custom Name", 1);
        AddHeader("Device ID", 2);
        AddHeader("Keycode", 3);

        Controls.Add(table);

        // Register for raw input
        RegisterRawInput();

        // Find and list keyboards
        ListKeyboards();
    }

    private void RegisterRawInput()
    {
        var device = new RAWINPUTDEVICE
        {
            usUsagePage = 0x01,
            usUsage = 0x06,
            dwFlags = 0x00000100, // RIDEV_INPUTSINK
            hwndTarget = Handle
        };

        if (!RegisterRawInputDevices(new RAWINPUTDEVICE[] { device }, 1, Marshal.SizeOf(typeof(RAWINPUTDEVICE))))
        {
            MessageBox.Show("Failed to register raw input device");
        }
    }

    private string? GetDeviceName(IntPtr device)
    {
        uint size = 0;
        GetRawInputDeviceInfo(device, 0x20000007, IntPtr.Zero, ref size);

        if (size == 0) return null;

        var namePtr = Marshal.AllocHGlobal((int)size);
        try
        {
            GetRawInputDeviceInfo(device, 0x20000007, namePtr, ref size);
            string devicePath = Marshal.PtrToStringUni(namePtr) ?? string.Empty;

            // Convert the raw device path into a more readable format
            // Example input: \\?\HID#VID_046D&PID_C534&MI_00#8&389ab617&0&0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}
            // We'll extract the VID (Vendor ID) and PID (Product ID)
            if (devicePath.Contains("VID_") && devicePath.Contains("PID_"))
            {
                int vidIndex = devicePath.IndexOf("VID_");
                int pidIndex = devicePath.IndexOf("PID_");

                if (vidIndex != -1 && pidIndex != -1)
                {
                    string vid = devicePath.Substring(vidIndex, 8); // VID_XXXX
                    string pid = devicePath.Substring(pidIndex, 8); // PID_XXXX
                    return $"Keyboard ({vid} {pid})";
                }
            }

            return devicePath;
        }
        finally
        {
            Marshal.FreeHGlobal(namePtr);
        }
    }

    private void AddHeader(string text, int column)
    {
        var header = new Label
        {
            Text = text,
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleCenter,
            Font = new Font(Font, FontStyle.Bold),
            BackColor = Color.LightGray
        };
        table.Controls.Add(header, column, 0);
    }

    private void ListKeyboards()
    {
        uint deviceCount = 0;
        GetRawInputDeviceList(IntPtr.Zero, ref deviceCount, (uint)Marshal.SizeOf(typeof(RAWINPUTDEVICELIST)));

        if (deviceCount == 0) return;

        var deviceList = Marshal.AllocHGlobal((int)(deviceCount * Marshal.SizeOf(typeof(RAWINPUTDEVICELIST))));
        try
        {
            GetRawInputDeviceList(deviceList, ref deviceCount, (uint)Marshal.SizeOf(typeof(RAWINPUTDEVICELIST)));

            for (int i = 0; i < deviceCount; i++)
            {
                var device = Marshal.PtrToStructure<RAWINPUTDEVICELIST>(
                    IntPtr.Add(deviceList, i * Marshal.SizeOf(typeof(RAWINPUTDEVICELIST))));

                if (device.dwType == RIM_TYPEKEYBOARD)
                {
                    var name = GetDeviceName(device.hDevice);
                    AddKeyboardRow(device.hDevice, name ?? "Unknown Keyboard");
                }
            }
        }
        finally
        {
            Marshal.FreeHGlobal(deviceList);
        }
    }

    private void AddKeyboardRow(IntPtr deviceHandle, string deviceName)
    {
        var row = new TableRow();
        row.DeviceId.Text = deviceHandle.ToString();
        row.CustomName.Text = deviceName;
        row.Keycode.Text = "No key pressed";

        // Add new row to table
        table.RowCount++;
        int rowIndex = table.RowCount - 1;

        // Add controls to table
        table.Controls.Add(row.Override, 0, rowIndex);
        table.Controls.Add(row.CustomName, 1, rowIndex);
        table.Controls.Add(row.DeviceId, 2, rowIndex);
        table.Controls.Add(row.Keycode, 3, rowIndex);

        deviceRows[deviceHandle] = row;
    }

    protected override void WndProc(ref Message message)
    {
        if (message.Msg == WM_INPUT)
        {
            uint size = 0;
            GetRawInputData(message.LParam, 0x10000003, IntPtr.Zero, ref size, (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER)));

            var buffer = Marshal.AllocHGlobal((int)size);
            try
            {
                if (GetRawInputData(message.LParam, 0x10000003, buffer, ref size, (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER))) == size)
                {
                    var input = Marshal.PtrToStructure<RAWINPUT>(buffer);
                    if (input.header.dwType == RIM_TYPEKEYBOARD)
                    {
                        if (deviceRows.TryGetValue(input.header.hDevice, out var row))
                        {
                            row.Keycode.Text = $"VK: {input.keyboard.VKey:X2}";
                            row.Flash();
                        }
                    }
                }
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }
        base.WndProc(ref message);
    }

    // Required structures and imports
    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTDEVICELIST
    {
        public IntPtr hDevice;
        public uint dwType;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTDEVICE
    {
        public ushort usUsagePage;
        public ushort usUsage;
        public uint dwFlags;
        public IntPtr hwndTarget;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTHEADER
    {
        public uint dwType;
        public uint dwSize;
        public IntPtr hDevice;
        public IntPtr wParam;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWKEYBOARD
    {
        public ushort MakeCode;
        public ushort Flags;
        public ushort Reserved;
        public ushort VKey;
        public uint Message;
        public uint ExtraInformation;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUT
    {
        public RAWINPUTHEADER header;
        public RAWKEYBOARD keyboard;
    }

    [DllImport("user32.dll")]
    private static extern bool RegisterRawInputDevices(RAWINPUTDEVICE[] pRawInputDevices, uint uiNumDevices, int cbSize);

    [DllImport("user32.dll")]
    private static extern uint GetRawInputDeviceList(IntPtr pRawInputDeviceList, ref uint puiNumDevices, uint cbSize);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    private static extern uint GetRawInputDeviceInfo(IntPtr hDevice, uint uiCommand, IntPtr pData, ref uint pcbSize);

    [DllImport("user32.dll")]
    private static extern uint GetRawInputData(IntPtr hRawInput, uint uiCommand, IntPtr pData, ref uint pcbSize, uint cbSize);
}

public class Program
{
    [STAThread]
    static void Main()
    {
        try
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error starting application: {ex.Message}\n\n{ex.StackTrace}");
        }
    }
}
