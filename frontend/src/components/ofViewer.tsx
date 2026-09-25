
export default function OfViewer() {
    return (
        <section style={{ width: '100%', padding: '0 0' }}>

            <iframe
                title="openFrameworksのペイント画面"
                src="/engine/index.html"
                allowFullScreen
                onLoad={(event) => {
                    const iframe = event.currentTarget;
                    const content =
                        iframe.contentDocument?.getElementById('engine-content');

                    if (!content) return;

                    const adjustHeight = () => {
                        const height = content.getBoundingClientRect().height;
                        iframe.style.height = `${Math.ceil(height)}px`;
                    };

                    adjustHeight();

                    const observer = new ResizeObserver(adjustHeight);
                    observer.observe(content);

                    iframe.contentWindow?.addEventListener(
                        'pagehide',
                        () => observer.disconnect(),
                        { once: true },
                    );
                }}
                style={{
                    display: 'block',
                    width: '1058px',
                    height: '600px',
                    border: 'none',
                    background: '#fff',
                }}
            />
        </section>
    )
}