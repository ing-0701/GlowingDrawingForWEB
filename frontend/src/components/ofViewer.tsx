export default function OfViewer(){
    return(
        <section style={{ width: '100%', padding: '24px 0' }}>

            <iframe
                title="openFrameworksのペイント画面"
                src="/engine/index.html"
                allowFullScreen
                style={{
                    display: 'block',
                    width: '100%',
                    height: '1100px',
                    border: '1px solid #888',
                    borderRadius: '8px',
                    background: '#fff',
                }}
            />
        </section>
    )
}